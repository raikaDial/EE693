// Ryker Dial
// EE693 Fall 2016
// Firmware for Teensy 3.6

#define ARM_MATH_CM4
#define __FPU_PRESENT 1
#include <arm_math.h>
#include "ECG_Filter.h"
#include "Filters.h"
#include <SPI.h>
#include <ADC.h>
//#include <IntervalTimer.h>
#include <TimerThree.h>
#include <wchar.h>
#include "ff.h"
#include <SparkFun_ADXL345.h>
#include "sleep.h"
#include "clocks.h"

// ***** DSP Stuff ***** //
ECG_Filter ecg_filter;
// Sign on 'a' coefficients is opposite of convention. Enter with caution.
iir_filter notch60Hz( (const float32_t[]){0.9582, 0.9582, 0.9582, 0.9582, 0.9163}, 1 );
iir_filter emgLP( (const float32_t[]){0.1453, 0.2906, 0.1453, 0.6710, -0.2533}, 1 );
// ********** //

// ***** Sensor Sampling Stuff ***** //
ADC *adc = new ADC(); // adc object
const float scale_timer = 11.7;
const uint32_t ECG_TS = (uint32_t)(1000000/360.0/scale_timer); // Sampling Period in Microseconds

float32_t emg_fs = 250;
const int ECG_PIN = A1;
volatile bool ecg_flag = 0;
// ********** //


// ***** MicroSD Card Stuff ***** //
FRESULT rc;        /* Result code */
FATFS fatfs;      /* File system object */
FIL fil;        /* File object */
DIR dir;        /* Directory object */
FILINFO fno;      /* File information object */

#define BUFFSIZE (2*1024)//(32*1024) // size of buffer to be written
#define ECG_BUFFSIZE 1024
uint8_t buffer[BUFFSIZE] __attribute__( ( aligned ( 16 ) ) );
uint16_t ecg_buff[ECG_BUFFSIZE] __attribute__( ( aligned ( 16 ) ) );
uint16_t ecg_buff_idx = 0;
// ********** //

// ***** ADXL345 Stuff ***** //
const uint16_t ADXL_CS_PIN = 10;
ADXL345 adxl = ADXL345(10);
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
    // Wait for serial to open. No need for Serial.begin() because Teensy has native USB
    //while (!Serial);
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

    rc = f_open(&fil, (TCHAR*)_T("test.bin"), FA_WRITE | FA_CREATE_ALWAYS);
    rc = f_close(&fil);

    Timer3.initialize(ECG_TS);
    Timer3.attachInterrupt(ECG_ISR);
}

void loop() {
    while(true) {
        sleep();
        noInterrupts();
        //digitalWriteFast(23, HIGH);
        if(ecg_flag) {
            ecg_buff[ecg_buff_idx++%ECG_BUFFSIZE] = adc->adc0->analogRead(ECG_PIN);
            ecg_flag = false;
        }
        if (ecg_buff_idx%ECG_BUFFSIZE == 0) {
          digitalWriteFast(23, HIGH);
            rc = f_open(&fil, (TCHAR*)_T("test.bin"), FA_WRITE | FA_OPEN_EXISTING);
            rc = f_lseek(&fil, f_size(&fil));
            
            for(int i=0; i<ECG_BUFFSIZE; ++i) {
                buffer[2*i] = (uint8_t)(ecg_buff[i]);
                buffer[2*i+1] = (uint8_t)(ecg_buff[i] >> 8);
            }
            rc = f_write(&fil, buffer, BUFFSIZE, &wr);
            rc = f_close(&fil);
            digitalWriteFast(23, LOW);  
        }
       
        //digitalWriteFast(23, LOW);
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
    ecg_flag = true;
}

// ********** //
