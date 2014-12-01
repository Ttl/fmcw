/* ###################################################################
**     Filename    : Events.c
**     Project     : fmcw
**     Processor   : MKL26Z128VFT4
**     Component   : Events
**     Version     : Driver 01.00
**     Compiler    : GNU C Compiler
**     Date/Time   : 2014-10-21, 09:14, # CodeGen: 0
**     Abstract    :
**         This is user's event module.
**         Put your event handler code here.
**     Settings    :
**     Contents    :
**         Cpu_OnNMIINT - void Cpu_OnNMIINT(void);
**
** ###################################################################*/
/*!
** @file Events.c
** @version 01.00
** @brief
**         This is user's event module.
**         Put your event handler code here.
*/         
/*!
**  @addtogroup Events_module Events module documentation
**  @{
*/         
/* MODULE Events */

#include "Cpu.h"
#include "Events.h"

#ifdef __cplusplus
extern "C" {
#endif 


/* User includes (#include below this line is not maintained by Processor Expert) */
#include "main.h"

/*
** ===================================================================
**     Event       :  Cpu_OnNMIINT (module Events)
**
**     Component   :  Cpu [MKL26Z256MC4]
*/
/*!
**     @brief
**         This event is called when the Non maskable interrupt had
**         occurred. This event is automatically enabled when the [NMI
**         interrupt] property is set to 'Enabled'.
*/
/* ===================================================================*/
void Cpu_OnNMIINT(void)
{
  /* Write your code here ... */
}

static void sawtooth(void) {
	(void)DA1_SetValue(&dac_out);
	if (dac_out <= dac_min) {
		meas_start = 1;
	}
	dac_out += DAC_STEP;
	if ( dac_out >= dac_max) {
		dac_out = dac_min;
	}
}

static void triangle(void) {
	static uint8_t up = 1;
	(void)DA1_SetValue(&dac_out);
	if (dac_out <= dac_min) {
		meas_start = 1;
		up = 1;
	}
	if (up) {
		dac_out += DAC_STEP;
	} else {
		dac_out -= DAC_STEP;
	}
	if ( dac_out >= dac_max) {
		up = 0;
	}
}

/*
** ===================================================================
**     Event       :  TI1_OnInterrupt (module Events)
**
**     Component   :  TI1 [TimerInt]
**     Description :
**         When a timer interrupt occurs this event is called (only
**         when the component is enabled - <Enable> and the events are
**         enabled - <EnableEvent>). This event is enabled only if a
**         <interrupt service/event> is enabled.
**     Parameters  : None
**     Returns     : Nothing
** ===================================================================
*/
void TI1_OnInterrupt(void)
{
	if (radar_mode == FMCW) {
#ifdef SAWTOOTH
		sawtooth();
#elif defined(TRIANGLE)
		triangle();
#else
#error no modulation defined
#endif
	} else {
		if (adc_buff_i >= DOPPLER_LENGTH ) {
			stop_meas();
		}
	}
}

/*
void TI1_OnInterrupt(void)
{
	static uint8_t up = 0;
#if DAC_STEP_MOD != 0
	static unsigned int i = 0;
#endif
	if (radar_mode == FMCW) {
		if (dac_out <= dac_min) {
			meas_start = 1;
			up = 1;
		}
		if ( dac_out >= dac_max) {
			up = 0;
		}
		(void)DA1_SetValue(&dac_out);
		if (up) {
			dac_out += DAC_STEP;
		} else {
			dac_out -= DAC_STEP;
		}
#if DAC_STEP_MOD != 0
		if (i++ % DAC_STEP_MOD == 0) {
			dac_out++;
		}
#endif
	} else {
		if (adc_buff_i >= ADC_BUFF_SIZE-1 ) {
			stop_meas();
		}
	}
}*/

/*
** ===================================================================
**     Event       :  CI2C1_OnReceiveData (module Events)
**
**     Component   :  CI2C1 [InternalI2C]
**     Description :
**         This event is invoked when I2C finishes the reception of the
**         data successfully. This event is not available for the SLAVE
**         mode and if both RecvChar and RecvBlock are disabled. This
**         event is enabled only if interrupts/events are enabled.
**     Parameters  : None
**     Returns     : Nothing
** ===================================================================
*/
void CI2C1_OnReceiveData(void)
{
  /* Write your code here ... */
}

/*
** ===================================================================
**     Event       :  CI2C1_OnTransmitData (module Events)
**
**     Component   :  CI2C1 [InternalI2C]
**     Description :
**         This event is invoked when I2C finishes the transmission of
**         the data successfully. This event is not available for the
**         SLAVE mode and if both SendChar and SendBlock are disabled.
**         This event is enabled only if interrupts/events are enabled.
**     Parameters  : None
**     Returns     : Nothing
** ===================================================================
*/
void CI2C1_OnTransmitData(void)
{
  /* Write your code here ... */
}

/*
** ===================================================================
**     Event       :  USB1_OnError (module Events)
**
**     Component   :  USB1 [FSL_USB_Stack]
**     Description :
**         Hook called in case of an error
**     Parameters  :
**         NAME            - DESCRIPTION
**         error           - Error code
**     Returns     : Nothing
** ===================================================================
*/
void USB1_OnError(byte error)
{
	stop_meas();
}

/* END Events */

#ifdef __cplusplus
}  /* extern "C" */
#endif 

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
