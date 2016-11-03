// Ryker Dial
// EE693 Fall 2016
// Firmware for Teensy 3.6

// ***** DSP Stuff ***** //
#define ARM_MATH_CM4
#define __FPU_PRESENT 1
#include <arm_math.h>
#include "ECG_Filter.h"
#include "Filters.h"

ECG_Filter ecg_filter;
// Sign on 'a' coefficients is opposite of convention. Enter with caution.
iir_filter notch60Hz( (const float32_t[]){0.9582, 0.9582, 0.9582, 0.9582, 0.9163}, 1 );
iir_filter emgLP( (const float32_t[]){0.1453, 0.2906, 0.1453, 0.6710, -0.2533}, 1 );
// ********** //

// ***** Sensor Sampling Stuff ***** //
#include <SPI.h>
#include <ADC.h>
#include <IntervalTimer.h>
ADC *adc = new ADC(); // adc object

IntervalTimer ecg_timer;
const float scale_timer = 40.8; // Divide clock for lower frequency of sleep.
const uint32_t ECG_TS = (uint32_t)(1000000/360.0/scale_timer); // Sampling Period in Microseconds

float32_t emg_fs = 250;
const int ECG_PIN = A1;
bool ecg_flag = 0;
// ********** //

// ***** MicroSD Card Stuff ***** //
#include <wchar.h>
#include "ff.h"

FRESULT rc;        /* Result code */
FATFS fatfs;      /* File system object */
FIL fil;        /* File object */
DIR dir;        /* Directory object */
FILINFO fno;      /* File information object */
UINT bw, br;
char buff[128];
TCHAR wbuff[128];

TCHAR * char2tchar( char * charString, size_t nn, TCHAR * tcharString) {
    for (size_t ii = 0; ii < nn; ii++) tcharString[ii] = (TCHAR) charString[ii];
    return tcharString;
}

char * tchar2char(  TCHAR * tcharString, size_t nn, char * charString) {
    for(size_t ii = 0; ii<nn; ii++) charString[ii] = (char) tcharString[ii];
    return charString;
}
// ********** //

// ***** ADXL345 Stuff ***** //
#include <SparkFun_ADXL345.h>
const uint16_t ADXL_CS_PIN = 10;
ADXL345 adxl = ADXL345(10);
// ********** //

// ***** Low Power Mode Stuff ***** //
#include <Snooze.h> // Using only for internal functions
void sleep() {
    pee_blpi( );
    enter_vlpr( );
    vlpw( );
    exit_vlpr( );
    blpi_pee( );    
}
// ********** //

void setup() {
    // Wait for serial to open. No need for Serial.begin() because Teensy has native USB
    while (!Serial);
    pinMode(23, OUTPUT);
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
}

void loop() {
    while(true) {
        ecg_timer.begin(ECG_ISR, ECG_TS);
        sleep();
        noInterrupts();
        digitalWriteFast(23, HIGH);
        if(ecg_flag) {
          uint16_t val = adc->adc0->analogRead(ECG_PIN);
          ecg_flag = false;
        }
        digitalWriteFast(23, LOW);
        interrupts();
    }
}

// ***** Interrupt Service Routines ***** //
void ADXL_ISR() {
  // getInterruptSource clears all triggered actions after returning value
  // Do not call again until you need to recheck for triggered actions
  byte interrupts = adxl.getInterruptSource();

  if(adxl.triggered(interrupts, ADXL345_FREE_FALL)){
  } 
  
  if(adxl.triggered(interrupts, ADXL345_INACTIVITY)){
  }
  
  if(adxl.triggered(interrupts, ADXL345_ACTIVITY)){
  }
  
  if(adxl.triggered(interrupts, ADXL345_DOUBLE_TAP)){
  }
  
  if(adxl.triggered(interrupts, ADXL345_SINGLE_TAP)){
  } 
}

void ECG_ISR(void) {
    ecg_timer.end();
    ecg_flag = true;
}

// ********** //
