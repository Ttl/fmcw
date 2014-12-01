/*
 * main.h
 *
 *  Created on: Oct 9, 2014
 *      Author: Henrik
 */

#ifndef MAIN_H_
#define MAIN_H_

extern volatile uint16_t dac_out;
extern uint8_t adc_err;
extern volatile uint8_t meas_start;

/* RUN, measurement running.
 * END, measurement ended, but data not transferred
 * STOP, data transferred.
 */
typedef volatile enum {START, RUN, END, STOP} meas_enum;
typedef volatile enum {DOPPLER, FMCW} radar_enum;

extern meas_enum meas_status;
extern radar_enum radar_mode;

extern volatile uint16_t dac_min;
extern volatile uint16_t dac_max;

//#define DAC_MIN 819 //2V = 5.6GHz
#define DAC_MIN_INIT 2257
#define DAC_MAX_INIT 2757
#define DAC_STEP 1
#define SAWTOOTH

#define ADC_BUFF_SIZE 3000
#define DOPPLER_LENGTH 2000

#define AUTOGAIN


extern uint16_t adc_buff[ADC_BUFF_SIZE];
extern volatile uint16_t adc_buff_i;

void stop_meas(void);
void start_meas(void);

#endif /* MAIN_H_ */
