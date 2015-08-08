/* ###################################################################
**     Filename    : main.c
**     Project     : fmcw
**     Processor   : MKL26Z128VFT4
**     Version     : Driver 01.01
**     Compiler    : GNU C Compiler
**     Date/Time   : 2014-10-21, 09:14, # CodeGen: 0
**     Abstract    :
**         Main module.
**         This module contains user's application code.
**     Settings    :
**     Contents    :
**         No public methods
**
** ###################################################################*/
/*!
** @file main.c
** @version 01.01
** @brief
**         Main module.
**         This module contains user's application code.
*/         
/*!
**  @addtogroup main_module main module documentation
**  @{
*/         
/* MODULE main */


/* Including needed modules to compile this module/procedure */
#include "Cpu.h"
#include "Events.h"
#include "AD1.h"
#include "AdcLdd1.h"
#include "CI2C1.h"
#include "IntI2cLdd1.h"
#include "DA1.h"
#include "DacLdd1.h"
#include "USB1.h"
#include "USB0.h"
#include "CDC1.h"
#include "Tx1.h"
#include "Rx1.h"
#include "CS1.h"
#include "WAIT1.h"
#include "UTIL1.h"
#include "TI1.h"
#include "TimerIntLdd1.h"
#include "TU1.h"
/* Including shared modules, which are used for whole project */
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"
/* User includes (#include below this line is not maintained by Processor Expert) */
#include "main.h"

#define PA_BIT (1U << 2)
#define PA_ON() (GPIOB_PCOR = PA_BIT)
#define PA_OFF() (GPIOB_PSOR = PA_BIT)

#define MCP_I2C_ADDR 0x2F
/* Step size for autogain adjustment */
#define MCP_STEP 5


#define max(x,y) ((x)>(y)?(x):(y))
#define abs(x) ((x)>0?(x):(-x))

volatile uint16_t dac_out = 0;
static uint8_t cdc_buffer[USB1_DATA_BUFF_SIZE];
static uint8_t in_buffer[USB1_DATA_BUFF_SIZE];

uint8_t adc_err;

volatile uint8_t meas_start;

uint8_t mcp_val = 0, old_mcp_val = 0;

/* ADC value storage */
uint16_t adc_buff[ADC_BUFF_SIZE];
volatile uint16_t adc_buff_i = 0;

volatile uint16_t dac_min = DAC_MIN_INIT;
volatile uint16_t dac_max = DAC_MAX_INIT;

uint16_t skip_samples = 200;

/* Status of the measurement */
meas_enum meas_status = STOP;
/* Radar mode */
radar_enum radar_mode = FMCW;

extern LDD_TDeviceData *AdcLdd1_DeviceDataPtr;

static uint8_t set_mcp(uint8_t val);
static void process_input(void);

void stop_meas(void) {
	PA_OFF();
	TI1_Disable();
	meas_status = STOP;
}

void start_meas(void) {
	//PA_ON();
	adc_err = ERR_OK;
	adc_buff_i = 0;
	//dac_out = dac_min;
	//TI1_Enable();
	meas_status = RUN;
	meas_start = 0;
	if (radar_mode == DOPPLER) {
		meas_start = 1;
	}
	while (!meas_start);
	meas_start = 0;
	uint16_t val_max = 0;
	uint16_t val_min = ~0;

	while( meas_status == RUN && adc_buff_i < ADC_BUFF_SIZE && !meas_start) {
		AD1_Measure(0); /* Start conversion */
		/* Skip first few samples, because we don't really care about them
		 * since they often have much higher amplitude than other samples */
		CDC1_App_Task(cdc_buffer, sizeof(cdc_buffer));
		if (adc_buff_i > skip_samples) {
				uint16_t val = adc_buff[adc_buff_i-1];
				if (val > val_max) {
					val_max = val;
				}
				if (val < val_min) {
					val_min = val;
				}
		}
		/* Wait for ADC to finish */
		while (AD1_GetValue16(&adc_buff[adc_buff_i]) != ERR_OK);
		adc_buff_i++;
	}
	//PA_OFF();
	/* Store the old gain for logging */
	old_mcp_val = mcp_val;
	/* Automatic gain control */
#ifdef AUTOGAIN
	if (radar_mode == FMCW) {
		if ( (mcp_val > 0) && ((val_max > 64000) || (val_min < 1000))) {
			if (mcp_val >= MCP_STEP) {
				mcp_val -= MCP_STEP;
			} else {
				mcp_val = 0;
			}
			set_mcp(mcp_val);
		} else if ( (mcp_val < 255) && ((val_max < 60000) && (val_min > 5000)) ) {
			if (mcp_val <= 255-MCP_STEP) {
					mcp_val += MCP_STEP;
			} else {
					mcp_val = 255;
			}
			set_mcp(mcp_val);
		}
	}
#endif
	meas_status = END;
}

void GPIO_init() {
	SIM_SCGC5 |= SIM_SCGC5_PORTB_MASK; //Clock on

	PORTB_PCR2 = PORT_PCR_MUX(0x1); //Set the PTB2 pin multiplexer to GPIO mode
	PA_OFF(); //Set value
	GPIOB_PDDR = PA_BIT; //Set direction
}

static uint8_t set_mcp(uint8_t val) {
	/* Set MCP4017 resistance.
	 * val = 0x00 ... 0x7F.
	 * 0x00 = 0 resistance.
	 * 0x7F = 10k resistance. */
	CI2C1_SelectSlave(MCP_I2C_ADDR);
	return CI2C1_SendChar(val);
	//return 0x01;
}

static void process_input(void) {
	unsigned int i = 0;
	do {
		if (CDC1_GetCharsInRxBuf()!=0) {
			  while(   i<sizeof(in_buffer)-1
					&& CDC1_GetChar(&in_buffer[i])==ERR_OK
				   )
			  {
				i++;
			  }
			  if(in_buffer[0] == 'O') {
				  i = 0;
				  (void)CDC1_SendString((unsigned char*)"PA ON\r\n");
				  PA_ON();
			  }
			  if(in_buffer[0] == 'o') {
				  i = 0;
				  (void)CDC1_SendString((unsigned char*)"PA OFF\r\n");
				  PA_OFF();
			  }
			  if(in_buffer[0] == 'm') {
				  i = 0;
				  if (meas_status != STOP) {
					  meas_status = STOP;
					  (void)CDC1_SendString((unsigned char*)"STOP\r\n");
				  } else {
					  dac_out = dac_min;
					  TI1_Enable();
					  meas_status = START;
					  (void)CDC1_SendString((unsigned char*)"START\r\n");
				  }
			  }
			  if(in_buffer[0] == 'e') {
				  i = 0;
				  (void)CDC1_SendChar((unsigned char)adc_err);
			  }
			  if(in_buffer[0] == 's' && i > 2) {
				  dac_min = (in_buffer[1]<<8)|in_buffer[2];
				  dac_out = dac_min;
				  i = 0;
			  }
			  if(in_buffer[0] == 'M' && i > 2) {
				  dac_max = (in_buffer[1]<<8)|in_buffer[2];
				  dac_out = dac_min;
				  i = 0;
			  }
			  if(in_buffer[0] == 'D') {
				  i = 0;
				  radar_mode = DOPPLER;
			  }
			  if(in_buffer[0] == 'F') {
				  i = 0;
				  radar_mode = FMCW;
			  }
			  if (in_buffer[0] == 'R' && i > 1) {
				  i = 0;
				  uint8_t val = in_buffer[1];
				  uint8_t ret = set_mcp(val);
				  (void)CDC1_SendChar((unsigned char)val);
			  }
			  if (i > 3) {
				  i = 0;
			  }
			  CDC1_App_Task(cdc_buffer, sizeof(cdc_buffer));
		}
	} while (i != 0);
}

static void meas_comm(void) {
    /* Measurement has ended. Send the data to PC */
    if (meas_status == END) {
    	meas_status = START;
		uint16_t samples = adc_buff_i-skip_samples;
		(void)CDC1_SendChar((unsigned char)'M');
		(void)CDC1_SendChar((unsigned char)(old_mcp_val));
		(void)CDC1_SendChar((unsigned char)(samples>>8));
		(void)CDC1_SendChar((unsigned char)(samples&0xFF));

    	if (1) {
			(void)CDC1_SendChar((unsigned char)(adc_buff[skip_samples]>>8));
			(void)CDC1_SendChar((unsigned char)(adc_buff[skip_samples]&0xFF));

			unsigned int i;
			for(i=skip_samples+1;i<adc_buff_i;i++) {
				int16_t diff = ((adc_buff[i]-adc_buff[i-1])>>1);
				if (diff > 127 || diff <= -128) {
					diff = 128;
					(void)CDC1_SendChar((unsigned char)(diff&0xFF));
					(void)CDC1_SendChar((unsigned char)(adc_buff[i]>>8));
					(void)CDC1_SendChar((unsigned char)(adc_buff[i]&0xFF));
				} else {
					if (diff < 0) {
						diff = 256+diff;
					}
					if (CDC1_SendChar((unsigned char)(diff&0xFF)) != ERR_OK) {
						stop_meas();
						break;
					}
				}
			}
    	} else {
			unsigned int i;
			for(i=skip_samples;i<adc_buff_i;i++) {
				(void)CDC1_SendChar((unsigned char)(adc_buff[i]>>8));
				(void)CDC1_SendChar((unsigned char)(adc_buff[i]&0xFF));
			}
    	}
    }
    if (meas_status == START) {
    	TI1_Enable();
    	start_meas();
    }
}

static void CDC_Run(void) {
	unsigned int loops = 0;
    unsigned int i = 0;
  (void)CDC1_SendString((unsigned char*)"FMCW radar started\r\n");
  for(;;) {
    while(CDC1_App_Task(cdc_buffer, sizeof(cdc_buffer))==ERR_BUSOFF) {
      /* Device not enumerated */
      WAIT1_Waitms(10);
      stop_meas();
    }
    meas_comm();
    process_input();
  }
}

/*lint -save  -e970 Disable MISRA rule (6.3) checking. */
int main(void)
/*lint -restore Enable MISRA rule (6.3) checking. */
{
  /* Write your local variable definition here */

  /*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/
  PE_low_level_init();
  /*** End of Processor Expert internal initialization.                    ***/
  GPIO_init();
  stop_meas();
  set_mcp(0);

  PE_AD1_Calibrate();

  CDC_Run();
  /* Write your code here */
  /* For example: for(;;) { } */

  /*** Don't write any code pass this line, or it will be deleted during code generation. ***/
  /*** RTOS startup code. Macro PEX_RTOS_START is defined by the RTOS component. DON'T MODIFY THIS CODE!!! ***/
  #ifdef PEX_RTOS_START
    PEX_RTOS_START();                  /* Startup of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of RTOS startup code.  ***/
  /*** Processor Expert end of main routine. DON'T MODIFY THIS CODE!!! ***/
  for(;;){}
  /*** Processor Expert end of main routine. DON'T WRITE CODE BELOW!!! ***/
} /*** End of main routine. DO NOT MODIFY THIS TEXT!!! ***/

/* END main */
/*!
** @}
*/
/*
** ###################################################################
**
**     This file was created by Processor Expert 10.4 [05.10]
**     for the Freescale Kinetis series of microcontrollers.
**
** ###################################################################
*/
