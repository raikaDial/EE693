// Ryker Dial
// EE693 Fall 2016
// Firmware for Teensy 3.6

#include "Arduino.h"
#define ARM_MATH_CM4
#define __FPU_PRESENT 1
#include <arm_math.h>
//#include "ECG_Filter.h"
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

// ***** Sensor Sampling Stuff ***** //
ADC *adc = new ADC(); // adc object
const int ECG_PIN = A1;
const int EMG_PIN = A2;

MAX30100 spo2_sensor;

// Timer Control
const float scale_timer = 15.4;
const uint32_t sampling_timer_TS = (uint32_t)(1000000/250.0/scale_timer); // Sampling Period in Microseconds
const uint32_t SPO2_SCALE = 25; // Sample at 10 Hz, so 1/25 everything else
volatile uint16_t spo2_counter = 0; // Keep track of when to sample SPO2

// ISR Flags
volatile bool ecg_flag = 0;
volatile bool adxl345_flag = 0;
volatile bool emg_flag = 0;

void sampling_timer_isr(void) {
    ecg_flag = true;
    adxl345_flag = true;
    emg_flag = true;
    spo2_sensor.readFifoData();
}

// Buffers for sensors and their respective control variables
#define NUMSAMPLES 1000
#define ADXL345_BUFFSIZE 3*1000
#define ECG_BUFFSIZE 1000
#define EMG_BUFFSIZE 1000
#define SPO2_BUFFSIZE 40
//uint16_t ecg_buff[ECG_BUFFSIZE] __attribute__( ( aligned ( 16 ) ) );
float32_t ecg_buff[ECG_BUFFSIZE] __attribute__( ( aligned ( 16 ) ) );
float32_t ecg_buff_filt[ECG_BUFFSIZE] __attribute__( ( aligned ( 16 ) ) );
float32_t emg_buff[EMG_BUFFSIZE] __attribute__( ( aligned ( 16 ) ) );
float32_t emg_buff_filt[ECG_BUFFSIZE] __attribute__( ( aligned ( 16 ) ) );
uint8_t spo2_buff[SPO2_BUFFSIZE] __attribute__( ( aligned ( 16 ) ) );
int16_t adxl345_buff[ADXL345_BUFFSIZE] __attribute__( ( aligned ( 16 ) ) );
uint16_t ecg_buff_idx = 0;
uint16_t adxl345_buff_idx = 0;
uint16_t spo2_buff_idx = 0;
uint16_t emg_buff_idx = 0;
// ********** //

// ***** DSP Stuff ***** //
// Sign on 'a' coefficients is opposite of convention. Enter with caution.

iir_filter notch60Hz( (const float32_t[]){0.9408, -0.1181, 0.9408, 0.1181, 0.8816}, 1 ); // fs = 250 Hz
iir_filter emgLP( (const float32_t[]){0.0021, 0.0042, 0.0021, 1.8669, -0.8752}, 1 ); // fs = 250 Hz

// ECG Filters
iir_filter ecgBP( (const float32_t[]){0.1122, 0, -0.1122, 1.7336, -0.7757}, 1 );// fs = 250 Hz, Passband = 5 - 15 Hz
fir_filter ecgDev( (const float32_t[]){-1, -2, 0, 2, 1}, 5, NUMSAMPLES); // ECG Derivative Filter
float32_t ecgMA_coeffs[37] = {
    1/37, 1/37, 1/37, 1/37, 1/37, 1/37, 1/37, 1/37, 1/37, 1/37, 1/37, 1/37,
    1/37, 1/37, 1/37, 1/37, 1/37, 1/37, 1/37, 1/37, 1/37, 1/37, 1/37, 1/37,
    1/37, 1/37, 1/37, 1/37, 1/37, 1/37, 1/37, 1/37, 1/37, 1/37, 1/37, 1/37,
    1/37
};
fir_filter ecgMA(ecgMA_coeffs, 37, NUMSAMPLES);
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

TCHAR * char2tchar( char * charString, size_t nn, TCHAR * tcharString) {
    for(size_t ii = 0; ii<nn; ii++) tcharString[ii] = (TCHAR) charString[ii];
    return tcharString;
}

#define BUFFSIZE 4*ECG_BUFFSIZE+4*EMG_BUFFSIZE+SPO2_BUFFSIZE+2*ADXL345_BUFFSIZE
uint8_t buffer[BUFFSIZE] __attribute__( ( aligned ( 16 ) ) );
//uint8_t magic_nums[] = {'E', 'E', '6', '9', '3'};
// ********** //

// ***** ADXL345 Stuff ***** //
const uint16_t ADXL_CS_PIN = 10;
ADXL345 adxl = ADXL345(10);
// ********** //

// ***** Pulse Oximeter Stuff ***** //

//
//// SaO2 Look-up Table
//// http://www.ti.com/lit/an/slaa274b/slaa274b.pdf
//const uint8_t spO2LUT[43] = {100,100,100,100,99,99,99,99,99,99,98,98,98,98,
//                             98,97,97,97,97,97,97,96,96,96,96,96,96,95,95,
//                             95,95,95,95,94,94,94,94,94,93,93,93,93,93};
// ********** //

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
    // // Wait for serial to open. No need for Serial.begin() because Teensy has native USB
    while (!Serial);
    pinMode(23, OUTPUT);
    pinMode(22, OUTPUT);
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
    adxl.setImportantInterruptMapping(1, 1, 1, 1, 1); // Put all interrupts on pin 1


    spo2_sensor.begin(); // Initialize Pulse Oximeter

    // Initialize Logger File
    for(size_t i=0; ;++i) {
        sprintf(fname, "log%u.bin", i);
        rc = f_open(&fil, char2tchar(fname, 128, wfname), FA_WRITE | FA_CREATE_NEW);
        if(rc == FR_OK) {
            rc = f_close(&fil);
            break;
        }
    }

    Timer3.initialize(sampling_timer_TS);
    Timer3.attachInterrupt(sampling_timer_isr);
}

void loop() {
    sleep();
    digitalWriteFast(23, HIGH);
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
    }
    if(Wire.spo2_flag) {
        digitalWriteFast(22, HIGH);
        uint16_t rawIRValue = Wire.spo2_bytes[0] << 8 | Wire.spo2_bytes[1];
        uint16_t rawRedValue = Wire.spo2_bytes[2] << 8 | Wire.spo2_bytes[3];

        Wire.spo2_flag = false;
        digitalWriteFast(22, LOW);
    }

    // Process and store collected data
    if (ecg_buff_idx%NUMSAMPLES == 0) {

        ecg_buff_idx = 0;
        emg_buff_idx = 0;
        adxl345_buff_idx = 0;
        spo2_buff_idx = 0;

        // ***** Data filtering goes here ***** //
        // ecgBP.filter(ecg_buff, ecg_buff_filt, NUMSAMPLES);
        // ecgDev.filter(ecg_buff_filt, ecg_buff_filt);
        // for(int i=0; i<NUMSAMPLES; ++i) // square the ecg signal
        //     ecg_buff_filt[i] = ecg_buff_filt[i]*ecg_buff_filt[i];
        ecgMA.filter(ecg_buff_filt, ecg_buff_filt);
        //emgLP.filter(emg_buff, emg_buff_filt, NUMSAMPLES);
        // ********** //

        // ***** Any processing using data goes here ***** //

        // ********** //

        rc = f_open(&fil, wfname, FA_WRITE | FA_OPEN_EXISTING);
        rc = f_lseek(&fil, f_size(&fil));

        memcpy(buffer, (uint8_t*)ecg_buff_filt, 4*ECG_BUFFSIZE);
        memcpy(buffer+4*ECG_BUFFSIZE, (uint8_t*)emg_buff_filt, 4*EMG_BUFFSIZE);
        memcpy(buffer+4*ECG_BUFFSIZE+4*EMG_BUFFSIZE, (uint8_t*)adxl345_buff, 2*ADXL345_BUFFSIZE);
        memcpy(buffer+4*ECG_BUFFSIZE+4*EMG_BUFFSIZE+2*ADXL345_BUFFSIZE, spo2_buff, SPO2_BUFFSIZE);
        rc = f_write(&fil, buffer, BUFFSIZE, &wr);
        rc = f_close(&fil);
    }
    interrupts();
    digitalWriteFast(23, LOW);
}

// ***** Interrupt Service Routines ***** //
// void ADXL_ISR() {
//   // getInterruptSource clears all triggered actions after returning value
//   // Do not call again until you need to recheck for triggered actions
//   byte interrupts = adxl.getInterruptSource();
//
//   if(adxl.triggered(interrupts, ADXL345_FREE_FALL)){
//   }
//
//   if(adxl.triggered(interrupts, ADXL345_INACTIVITY)){
//   }
//
//   if(adxl.triggered(interrupts, ADXL345_ACTIVITY)){
//   }
//
//   if(adxl.triggered(interrupts, ADXL345_DOUBLE_TAP)){
//   }
//
//   if(adxl.triggered(interrupts, ADXL345_SINGLE_TAP)){
//   }
// }


// ********** //
