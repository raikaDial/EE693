// Ryker Dial
// EE693 Fall 2016
// Firmware for Teensy 3.6

#include "Arduino.h"
#define ARM_MATH_CM4
#define __FPU_PRESENT 1
#include <arm_math.h>
#include "Filters.h"
#include <SPI.h>
#include <ADC.h>
#include "i2c_t3.h" // Modified to trigger SPO2 calculation on Wire interrupt
#include <TimerThree.h>
#include <wchar.h>
#include <ff.h>
#include <SparkFun_ADXL345.h>
#include "MAX30100.h"
#include "sleep.h"
#include "clocks.h"

#define RAWDATA_SAVE

// ***** Sensor Sampling Stuff ***** //
ADC *adc = new ADC(); // adc object
const int ECG_PIN = A1;
const int EMG_PIN = A7;

// Timer Control
const float scale_timer = 15.0; // Need to scale the sampling frequency to account for time spent in sleep mode. Since
                                //     the timers are based on the clock speed, and the clock speed goes from 180 MHz
                                //     to 2 MHz for sleep mode, need to manually fine tune this value to account for this.
const uint32_t sampling_timer_TS = (uint32_t)(1000000/250.0/scale_timer); // Sampling Period in Microseconds

// SPO2 Stuff
const uint32_t SPO2_SCALE = 5;  // Sample SPO2 sensor at 250/5 = 50 Hz.
const uint32_t SPO2_NUMVAR = 5; // Use 5 raw samples from the SPO2 sensor to calculate the variance of the IR and red
                                //     light and determine the SPO2 concentration. This puts actual SPO2 fs at 10 Hz.
MAX30100 spo2_sensor(SPO2_NUMVAR);
volatile uint16_t spo2_counter = 0; // Keep track of when to sample SPO2

// ISR Flags
volatile bool ecg_flag = 0;
volatile bool adxl345_flag = 0;
volatile bool emg_flag = 0;

void sampling_timer_isr(void) {
    ecg_flag = true;
    adxl345_flag = true;
    emg_flag = true;

    if(!(++spo2_counter%SPO2_SCALE))
        spo2_sensor.readFifoData();
}

// Buffers for sensors and their respective control variables.
#define NUMSAMPLES 1000
#define ADXL345_BUFFSIZE 3*1000
#define ECG_BUFFSIZE 1000
#define EMG_BUFFSIZE 1000
#define SPO2_BUFFSIZE 40
#define FLAG_BUFFSIZE 1000
float32_t ecg_buff[ECG_BUFFSIZE] __attribute__( ( aligned ( 16 ) ) );
float32_t ecg_buff_filt[ECG_BUFFSIZE] __attribute__( ( aligned ( 16 ) ) );
float32_t emg_buff[EMG_BUFFSIZE] __attribute__( ( aligned ( 16 ) ) );
float32_t emg_buff_filt[ECG_BUFFSIZE] __attribute__( ( aligned ( 16 ) ) );
uint8_t spo2_buff[SPO2_BUFFSIZE] __attribute__( ( aligned ( 16 ) ) );
int16_t adxl345_buff[ADXL345_BUFFSIZE] __attribute__( ( aligned ( 16 ) ) );
uint8_t flag_buff[FLAG_BUFFSIZE] __attribute__( ( aligned ( 16 ) ) );
uint16_t ecg_buff_idx = 0;
uint16_t adxl345_buff_idx = 0;
uint16_t spo2_buff_idx = 0;
uint16_t emg_buff_idx = 0;
uint16_t flag_buff_idx = 0;
uint8_t single_trigger_counter=0;
// ********** //

// ***** DSP Stuff ***** //
// Sign on 'a' coefficients is opposite of convention. Enter with caution.

// EMG Filters
iir_filter emgLP( (const float32_t[]){0.0021, 0.0042, 0.0021, 1.8669, -0.8752}, 1 ); // fs = 250 Hz
iir_filter emgBP( (const float32_t[]){0.4208, 0, -0.4208, 0.6096, -0.1584}, 1 ); // fs = 250 Hz Passband = 20 - 70 Hz

// ECG Filters
iir_filter notch60Hz( (const float32_t[]){0.9408, -0.1181, 0.9408, 0.1181, 0.8816}, 1 ); // fs = 250 Hz
iir_filter ecgBP( (const float32_t[]){0.1122, 0, -0.1122, 1.7336, -0.7757}, 1 );// fs = 250 Hz, Passband = 5 - 15 Hz
fir_filter ecgDev( (const float32_t[]){-1, -2, 0, 2, 1}, 5, NUMSAMPLES); // ECG Derivative Filter
float32_t ecgMA_coeffs[37] = {
    1/37.0, 1/37.0, 1/37.0, 1/37.0, 1/37.0, 1/37.0, 1/37.0, 1/37.0, 1/37.0, 1/37.0, 1/37.0, 1/37.0,
    1/37.0, 1/37.0, 1/37.0, 1/37.0, 1/37.0, 1/37.0, 1/37.0, 1/37.0, 1/37.0, 1/37.0, 1/37.0, 1/37.0,
    1/37.0, 1/37.0, 1/37.0, 1/37.0, 1/37.0, 1/37.0, 1/37.0, 1/37.0, 1/37.0, 1/37.0, 1/37.0, 1/37.0,
    1/37.0
};
fir_filter ecgMA(ecgMA_coeffs, 37, NUMSAMPLES); // Moving Average Filter
// ********** //

// ***** MicroSD Card Stuff ***** //
FRESULT rc;        /* Result code */
FATFS fatfs;      /* File system object */
FIL fil;        /* File object */
DIR dir;        /* Directory object */
FILINFO fno;      /* File information object */
UINT wr;
char fname[128];
TCHAR wfname[128];
#if defined(RAWDATA_SAVE)
    TCHAR wfname_raw[128];
#endif

TCHAR * char2tchar( char * charString, size_t nn, TCHAR * tcharString) {
    for(size_t ii = 0; ii<nn; ii++) tcharString[ii] = (TCHAR) charString[ii];
    return tcharString;
}

#define BUFFSIZE 4*ECG_BUFFSIZE+4*EMG_BUFFSIZE+SPO2_BUFFSIZE+2*ADXL345_BUFFSIZE+FLAG_BUFFSIZE
uint8_t buffer[BUFFSIZE] __attribute__( ( aligned ( 16 ) ) );
// ********** //

// ***** ADXL345 Stuff ***** //
const uint16_t ADXL_CS_PIN = 10;
ADXL345 adxl = ADXL345(ADXL_CS_PIN);

void ADXL_ISR() {
  // getInterruptSource clears all triggered actions after returning value
  // Do not call again until you need to recheck for triggered actions
  byte interrupts = adxl.getInterruptSource();

  if(adxl.triggered(interrupts, ADXL345_SINGLE_TAP)){
    //digitalWrite(23, HIGH);
    ++single_trigger_counter;
    flag_buff[flag_buff_idx]=single_trigger_counter;
    //digitalWrite(23, LOW);
  }
  else if (single_trigger_counter>0){
  --single_trigger_counter;
  flag_buff[flag_buff_idx]=single_trigger_counter;
}
}
// ***** Low Power Mode Stuff ***** //
void sleep() {
    pee_blpi( );
    enter_vlpr( );
    vlpw( );
    exit_vlpr( );
    blpi_pee( );
}
// ********** //

void setup() {
    f_mount(&fatfs, (TCHAR*)_T("/"), 0); /* Mount/Unmount a logical drive */

    // Setup ADC
    adc->setAveraging(1);
    adc->setResolution(10);
    adc->setConversionSpeed(ADC_HIGH_SPEED);
    adc->setSamplingSpeed(ADC_HIGH_SPEED);

    // Setup ADXL345
    SPI.setSCK(14); // Use alternate clock that is not tied to LED
    adxl.setSpiBit(0); // Use 4-wire SPI
    adxl.powerOn();
    adxl.setRangeSetting(16);
    adxl.setFullResBit(true);
    adxl.setImportantInterruptMapping(1, 1, 1, 1, 1); // Put all interrupts on pin 1
    adxl.setTapDetectionOnXYZ(1, 0, 0); // Detect taps in the directions turned ON "adxl.setTapDetectionOnX(X, Y, Z);" (1 == ON, 0 == OFF)

    // Set values for what is considered a TAP and what is a DOUBLE TAP (0-255)
    adxl.setTapThreshold(31);           // 62.5 mg per increment
    adxl.setTapDuration(240);            // 625 μs per increment
    adxl.setDoubleTapLatency(80);       // 1.25 ms per increment
    adxl.setDoubleTapWindow(200);       // 1.25 ms per increment
    adxl.singleTapINT(1);

    spo2_sensor.begin(); // Initialize Pulse Oximeter

    // Initialize Logger File
    for(size_t i=0; ;++i) {
        sprintf(fname, "log%u_filt.bin", i);
        rc = f_open(&fil, char2tchar(fname, 128, wfname), FA_WRITE | FA_CREATE_NEW);
        if(rc == FR_OK) {
            rc = f_close(&fil);
            #if defined(RAWDATA_SAVE)
                sprintf(fname, "log%u_raw.bin", i);
                rc = f_open(&fil, char2tchar(fname, 128, wfname_raw), FA_WRITE | FA_CREATE_NEW);
            #endif
            break;
        }
    }

    Timer3.initialize(sampling_timer_TS);
    Timer3.attachInterrupt(sampling_timer_isr);
}

void loop() {
    sleep();
    //digitalWriteFast(23, HIGH);
    noInterrupts();
    if(ecg_flag) {
        ecg_buff[ecg_buff_idx++] = (float32_t)(adc->adc0->analogRead(ECG_PIN));
        ecg_flag = false;
    }
    if(emg_flag) {
        emg_buff[emg_buff_idx++] = (float32_t)(adc->adc0->analogRead(EMG_PIN));
        emg_flag = false;
    }
    if(adxl345_flag) {
        adxl.readAccel(&adxl345_buff[adxl345_buff_idx]);
        adxl345_buff_idx +=3;
        adxl345_flag = false;
        flag_buff[flag_buff_idx] = 0;
        ADXL_ISR();
        ++flag_buff_idx;
    }
    if(Wire.spo2_flag) {
        // Read latest raw data
        uint16_t rawIRValue = Wire.spo2_bytes[0] << 8 | Wire.spo2_bytes[1];
        uint16_t rawRedValue = Wire.spo2_bytes[2] << 8 | Wire.spo2_bytes[3];

        // Remove DC Component from raw data
        spo2_sensor.irACValue[spo2_sensor.ac_idx] = spo2_sensor.irDCRemover.step((float)rawIRValue);
        spo2_sensor.redACValue[spo2_sensor.ac_idx] = spo2_sensor.redDCRemover.step((float)rawRedValue);

        // If we have collected enough samples, calculate SPO2
        if(!(++spo2_sensor.ac_idx%spo2_sensor.num_samples_var)) {
            spo2_sensor.ac_idx = 0;
            float irACValueSqSum = 0;
            float redACValueSqSum = 0;

            // Calculate ratio of variance
            for(int i=0; i<spo2_sensor.num_samples_var; ++i) {
                irACValueSqSum += (float)spo2_sensor.irACValue[i]*(float)spo2_sensor.irACValue[i];
                redACValueSqSum += (float)spo2_sensor.redACValue[i]*(float)spo2_sensor.redACValue[i];
            }
            float acSqRatio = 100.0*log(redACValueSqSum/((float)spo2_sensor.num_samples_var))/log(irACValueSqSum/((float)spo2_sensor.num_samples_var));

            // Determine index in SPO2 Lookup Table
            uint8_t index = 0;
            if (acSqRatio > 66)
                index = (uint8_t)acSqRatio - 66;
            else if (acSqRatio > 50)
                index = (uint8_t)acSqRatio - 50;

            if(index >= 0 && index <= 43) // Make sure index is within L
                spo2_buff[spo2_buff_idx++] = spo2_sensor.spO2LUT[index];
            else
                spo2_buff[spo2_buff_idx++] = 0;
        }

        Wire.spo2_flag = false;
    }

    // Process and store collected data
    if (ecg_buff_idx%NUMSAMPLES == 0) {

        ecg_buff_idx = 0;
        emg_buff_idx = 0;
        adxl345_buff_idx = 0;
        spo2_buff_idx = 0;
        flag_buff_idx = 0;

        // ***** Data filtering goes here ***** //
        ecgBP.filter(ecg_buff, ecg_buff_filt, NUMSAMPLES);
        ecgDev.filter(ecg_buff_filt, ecg_buff_filt);
        for(int i=0; i<NUMSAMPLES; ++i) // square the ecg signal
            ecg_buff_filt[i] = ecg_buff_filt[i]*ecg_buff_filt[i];
        ecgMA.filter(ecg_buff_filt, ecg_buff_filt);
        emgLP.filter(emg_buff, emg_buff_filt, NUMSAMPLES);
      //  emgBP.filter(emg_buff, emg_buff_filt, NUMSAMPLES);

        // ***** Any processing using data goes here ***** //

        rc = f_open(&fil, wfname, FA_WRITE | FA_OPEN_EXISTING);
        rc = f_lseek(&fil, f_size(&fil));

        // Save filtered data
        memcpy(buffer, (uint8_t*)ecg_buff_filt, 4*ECG_BUFFSIZE);
        memcpy(buffer+4*ECG_BUFFSIZE, (uint8_t*)emg_buff_filt, 4*EMG_BUFFSIZE);
        memcpy(buffer+4*ECG_BUFFSIZE+4*EMG_BUFFSIZE, (uint8_t*)adxl345_buff, 2*ADXL345_BUFFSIZE);
        memcpy(buffer+4*ECG_BUFFSIZE+4*EMG_BUFFSIZE+2*ADXL345_BUFFSIZE, spo2_buff, SPO2_BUFFSIZE);
        memcpy(buffer+4*ECG_BUFFSIZE+4*EMG_BUFFSIZE+2*ADXL345_BUFFSIZE+SPO2_BUFFSIZE, flag_buff, FLAG_BUFFSIZE);

        rc = f_write(&fil, buffer, BUFFSIZE, &wr);
        rc = f_close(&fil);

        #if defined(RAWDATA_SAVE)
            // Save unfiltered data just for testing
            rc = f_open(&fil, wfname_raw, FA_WRITE | FA_OPEN_EXISTING);
            rc = f_lseek(&fil, f_size(&fil));
            memcpy(buffer, (uint8_t*)ecg_buff, 4*ECG_BUFFSIZE);
            memcpy(buffer+4*ECG_BUFFSIZE, (uint8_t*)emg_buff, 4*EMG_BUFFSIZE);
            rc = f_write(&fil, buffer, BUFFSIZE, &wr);
            rc = f_close(&fil);
        #endif
    }
    interrupts();
}
