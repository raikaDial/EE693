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
#include <TimerThree.h>
#include <wchar.h>
#include "ff.h"
#include <SparkFun_ADXL345.h>
#include "sleep.h"
#include "clocks.h"

// ***** DSP Stuff ***** //
ECG_Filter ecg_filter;
// Sign on 'a' coefficients is opposite of convention. Enter with caution.
//iir_filter notch60Hz( (const float32_t[]){0.9582, 0.9582, 0.9582, 0.9582, 0.9163}, 1 ); // fs = 360,Hz
iir_filter notch60Hz( (const float32_t[]){0.9408, -0.1181, 0.9408, 0.1181, 0.8816}, 1 ); // fs = 250,Hz
iir_filter emgLP( (const float32_t[]){0.1453, 0.2906, 0.1453, 0.6710, -0.2533}, 1 ); // fs = 25- Hz
//
//iir_filter pan_tompkins_lp( 
//    (const float32_t[]) {
//        1.0000,    2.0000,    1.0000,      -2.0000,    1.0000,
//        1.0000,    1.0000,    1.0000,            0,        0,
//        1.0000,   -1.0000,    1.0000,            0,        0,
//        1.0000,   -2.0000,    1.0000,           0,        0,
//        1.0000,   -1.0000,    1.0000,          0,        0,
//        1.0000,    1.0000,    1.0000,           0,        0      
//    },
//    6
//);
//
//iir_filter pan_tompkins_hp( 
//    (const float32_t[]) {
//        1.0000,    0.2990,   -1.27230,     -1.0000,         0,
//        1.0000,    2.3862,    1.6535,             0,        0,
//        1.0000,    1.8497,    1.6424,            0,        0,
//        1.0000,    1.0475,    1.6230,           0,        0,
//        1.0000,    0.1031,    1.5938,           0,        0,
//        1.0000,   -0.8369,    1.5519,           0,        0,
//        1.0000,   -1.6236,    1.4908,            0,        0,
//        1.0000,   -2.1199,    1.3924,           0,        0,
//        1.0000,   -2.0000,    1.0000,          0,        0,
//        1.0000,   -1.4782,    0.6973,           0,        0,
//        1.0000,   -1.0040,    0.6487,           0,        0,
//        1.0000,   -0.4104,    0.6220,       0,        0,
//        1.0000,    0.2249,    0.6051,           0,        0,
//        1.0000,    0.8094,    0.5942,            0,        0,
//        1.0000,    1.2559,    0.5876,          0,        0,
//        1.0000,    1.4974,    0.5844,          0,        0   
//    },
//    16
//);
//iir_filter pan_tompkins_dev( 
//    (const float32_t[]) {
//        1.0000,    0.0000,   -1.0000,        0,        0,
//        1.0000,    0.5000,   1.0000,      0,        0
//    },
//    2
//);

// ********** //

// ***** Sensor Sampling Stuff ***** //
ADC *adc = new ADC(); // adc object

const float scale_timer = 15.98;
const uint32_t ECG_TS = (uint32_t)(1000000/250.0/scale_timer); // Sampling Period in Microseconds
const uint32_t SPO2_SCALE = 25; // Sample at 10 Hz, so 1/25 everything else
uint16_t spo2_counter = 0;

// Buffers for sensors and their respective control variables
#define ADXL345_BUFFSIZE 3*2*1000
#define ECG_BUFFSIZE 2*1000
#define EMG_BUFFSIZE 2*1000
#define SPO2_BUFFSIZE 40
uint8_t ecg_buff[ECG_BUFFSIZE] __attribute__( ( aligned ( 16 ) ) );
uint8_t emg_buff[EMG_BUFFSIZE] __attribute__( ( aligned ( 16 ) ) );
uint8_t spo2_buff[SPO2_BUFFSIZE] __attribute__( ( aligned ( 16 ) ) );
uint8_t adxl345_buff[ADXL345_BUFFSIZE] __attribute__( ( aligned ( 16 ) ) );
uint16_t ecg_buff_idx = 0;
uint16_t adxl345_buff_idx = 0;
uint16_t spo2_buff_idx = 0;
uint16_t emg_buff_idx = 0;

const int ECG_PIN = A1;
const int EMG_PIN = A2;
volatile bool ecg_flag = 0;
volatile bool adxl345_flag = 0;
volatile bool emg_flag = 0;
volatile bool spo2_flag = 0;
// ********** //

// ***** MicroSD Card Stuff ***** //
FRESULT rc;        /* Result code */
FATFS fatfs;      /* File system object */
FIL fil;        /* File object */
DIR dir;        /* Directory object */
FILINFO fno;      /* File information object */
UINT wr;

#define BUFFSIZE ECG_BUFFSIZE+EMG_BUFFSIZE+SPO2_BUFFSIZE+ADXL345_BUFFSIZE//2*ECG_BUFFSIZE+2*EMG_BUFFSIZE+SPO2_BUFFSIZE+2*ADXL345_BUFFSIZE // size of uSD Buffer.
uint8_t buffer[BUFFSIZE] __attribute__( ( aligned ( 16 ) ) );

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

    rc = f_open(&fil, (TCHAR*)_T("test.bin"), FA_WRITE | FA_CREATE_ALWAYS);
    rc = f_close(&fil);

//    Timer1.initialize(SPO2_TS);
//    Timer1.attachInterrupt(SPO2_ISR);
    Timer3.initialize(ECG_TS);
    Timer3.attachInterrupt(ECG_ISR);
}

void loop() {
    while(true) {
        sleep();
        noInterrupts();
        //digitalWriteFast(23, HIGH);
        if(ecg_flag) {
            //digitalWriteFast(22, HIGH);
            uint16_t val = adc->adc0->analogRead(ECG_PIN);
            ecg_buff[ecg_buff_idx] = (uint8_t) val;
            ecg_buff[ecg_buff_idx+1] = (uint8_t) (val >> 8);
            ecg_buff_idx+=2;
            //ecg_buff[ecg_buff_idx++] = adc->adc0->analogRead(ECG_PIN);
            ecg_flag = false;
           //digitalWriteFast(22, LOW);
        }
        if(adxl345_flag) {
            int16_t x, y, z;
            adxl.readAccel(&x, &y, &z);
            adxl345_buff[adxl345_buff_idx+0] = (uint8_t) x;
            adxl345_buff[adxl345_buff_idx+1] = (uint8_t) (x >> 8);
            adxl345_buff[adxl345_buff_idx+2] = (uint8_t) y;
            adxl345_buff[adxl345_buff_idx+3] = (uint8_t) (y >> 8);
            adxl345_buff[adxl345_buff_idx+4] = (uint8_t) z;
            adxl345_buff[adxl345_buff_idx+5] = (uint8_t) (z >> 8);                        
            //adxl.readAccel( &(adxl345_buff[adxl345_buff_idx]) );
            adxl345_buff_idx += 6;
            adxl345_flag = false;
        }
        if(emg_flag) {
            uint16_t val = adc->adc0->analogRead(EMG_PIN);
            emg_buff[emg_buff_idx] = (uint8_t) val;
            emg_buff[emg_buff_idx+1] = (uint8_t) (val >> 8);
            emg_buff_idx+=2;
            emg_flag = false;
        }
        if(spo2_flag) {
            spo2_flag = false;
        }
        // Process and store collected data
        if (ecg_buff_idx%ECG_BUFFSIZE == 0) {
            //digitalWriteFast(23, HIGH);
            ecg_buff_idx = 0;
            adxl345_buff_idx = 0;

            //***** Data filtering goes here *****//
            //ecg_filter.pan85_filter(ecg_buff, (size_t)ECG_BUFFSIZE);
            //**********//
            
            rc = f_open(&fil, (TCHAR*)_T("test.bin"), FA_WRITE | FA_OPEN_EXISTING);
            rc = f_lseek(&fil, f_size(&fil));

            memcpy(buffer, ecg_buff, ECG_BUFFSIZE);
            memcpy(buffer+ECG_BUFFSIZE, emg_buff, EMG_BUFFSIZE);
            memcpy(buffer+ECG_BUFFSIZE+EMG_BUFFSIZE, spo2_buff, SPO2_BUFFSIZE);
            memcpy(buffer+ECG_BUFFSIZE+EMG_BUFFSIZE+SPO2_BUFFSIZE, adxl345_buff, ADXL345_BUFFSIZE);
            // Store ECG Data
//            for(int i=0; i<(ECG_BUFFSIZE << 1); i+=2) {
//                buffer[i] = (uint8_t)(ecg_buff[(i >> 1)]);
//                buffer[i+1] = (uint8_t)(ecg_buff[(i >> 1)] >> 8);
//            }
//            // Store EMG Data
//            for(int i=(ECG_BUFFSIZE << 1); i<((ECG_BUFFSIZE+EMG_BUFFSIZE) << 1); i+=2) {
//                buffer[i] = (uint8_t)(emg_buff[(i >> 1)]);
//                buffer[i+1] = (uint8_t)(emg_buff[(i >> 1)] >> 8); 
//            }
//            // Store SPO2 Data
//            for(int i=((ECG_BUFFSIZE+EMG_BUFFSIZE) << 1); i<((ECG_BUFFSIZE+EMG_BUFFSIZE) << 1)+SPO2_BUFFSIZE; ++i) {
//                buffer[i] = spo2_buff[i];
//            }
//            // Store ADXL Data | x | y | z |
//            for(int i=((ECG_BUFFSIZE+EMG_BUFFSIZE) << 1)+SPO2_BUFFSIZE; 
//              i<((ECG_BUFFSIZE+EMG_BUFFSIZE+ADXL345_BUFFSIZE) << 1)+SPO2_BUFFSIZE; i+=6) {
//                buffer[i] = (uint8_t)(adxl345_buff[i]);
//                buffer[i+1] = (uint8_t)(adxl345_buff[i] >> 8);
//                buffer[i+2] = (uint8_t)(adxl345_buff[i+1]);
//                buffer[i+3] = (uint8_t)(adxl345_buff[i+1] >> 8);
//                buffer[i+4] = (uint8_t)(adxl345_buff[i+2]);
//                buffer[i+5] = (uint8_t)(adxl345_buff[i+2] >> 8);
//              }
            
                
//            for(int i=0; i<ECG_BUFFSIZE; ++i) {
//                // Store ECG Data
//                buffer[8*i] = (uint8_t)(ecg_buff[i]);
//                buffer[8*i+1] = (uint8_t)(ecg_buff[i] >> 8);
//                // Store EMG Data
//                buffer[8*i+2] = (uint8_t)(emg_buff[i]);
//                buffer[8*i+3] = (uint8_t)(emg_buff[i] >> 8);
//                // Store AXDL345 Data | x | y | z |
//                buffer[8*i+4] = (uint8_t)(adxl345_buff[3*i]);
//                buffer[8*i+5] = (uint8_t)(adxl345_buff[3*i] >> 8);
//                buffer[8*i+6] = (uint8_t)(adxl345_buff[3*i+1]);
//                buffer[8*i+7] = (uint8_t)(adxl345_buff[3*i+1] >> 8);
//                buffer[8*i+8] = (uint8_t)(adxl345_buff[3*i+2]);
//                buffer[8*i+9] = (uint8_t)(adxl345_buff[3*i+2] >> 8);
//            }
            
            rc = f_write(&fil, buffer, BUFFSIZE, &wr);
            rc = f_close(&fil);
            //digitalWriteFast(23, LOW);  
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
    adxl345_flag = true;
    emg_flag = true;
    if(!spo2_counter++%SPO2_SCALE) spo2_flag = true;
}

//void SPO2_ISR(void) {
//    spo2_flag = true;
//}
// ********** //
