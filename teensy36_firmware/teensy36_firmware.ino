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
//#include "i2c_t3.h"
#include <wchar.h>
#include "ff.h"
#include <SparkFun_ADXL345.h>
//#include "MAX30100.h"
#include "sleep.h"
#include "clocks.h"

// ***** DSP Stuff ***** //
ECG_Filter ecg_filter;
// Sign on 'a' coefficients is opposite of convention. Enter with caution.
iir_filter notch60Hz( (const float32_t[]){0.9408, -0.1181, 0.9408, 0.1181, 0.8816}, 1 ); // fs = 250 Hz
iir_filter emgLP( (const float32_t[]){0.1453, 0.2906, 0.1453, 0.6710, -0.2533}, 1 ); // fs = 250 Hz
iir_filter ecgBP( (const float32_t[]){0.1122, 0, -0.1122, 1.7336, -0.7757}, 1 );// fs = 250 Hz, Passband = 5 - 15 Hz
iir_filter emgHP_20( (const float32_t[]){.6998, -1.3995, 0.6998, 1.3073, -0.4918}, 1 ); // fs = 250 Hz
iir_filter emgLP_70( (const float32_t[]){0.3503, 0.7007, 0.3503, -0.2212, -0.1802}, 1 ); // fs = 250 Hz

// ********** //

// ***** Sensor Sampling Stuff ***** //
ADC *adc = new ADC(); // adc object
const int ECG_PIN = A1;
const int EMG_PIN = A2;

// Timer Control
const float scale_timer = 15.98;
const uint32_t ECG_TS = (uint32_t)(1000000/250.0/scale_timer); // Sampling Period in Microseconds
const uint32_t SPO2_SCALE = 25; // Sample at 10 Hz, so 1/25 everything else
volatile uint16_t spo2_counter = 0; // Keep track of when to sample SPO2

// ISR Flags
volatile bool ecg_flag = 0;
volatile bool adxl345_flag = 0;
volatile bool emg_flag = 0;
volatile bool spo2_flag = 0;
int sing_trig= 0;
int doub_trig= 0;
size_t choice=1; //decide 1 for only using a low and high pass filter and anything else for using all filters

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
    for(int ii = 0; ii<nn; ii++) tcharString[ii] = (TCHAR) charString[ii];
    return tcharString;
}

//#define BUFFSIZE 2*ECG_BUFFSIZE+2*EMG_BUFFSIZE+SPO2_BUFFSIZE+2*ADXL345_BUFFSIZE
#define BUFFSIZE 4*ECG_BUFFSIZE+2*EMG_BUFFSIZE+SPO2_BUFFSIZE+2*ADXL345_BUFFSIZE
uint8_t buffer[BUFFSIZE] __attribute__( ( aligned ( 16 ) ) );
//uint8_t magic_nums[] = {'E', 'E', '6', '9', '3'};

// ********** //

// ***** ADXL345 Stuff ***** //
const uint16_t ADXL_CS_PIN = 10;
ADXL345 adxl = ADXL345(10);
// ********** //

// ***** Pulse Oximeter Stuff ***** //
//MAX30100 spo2_sensor;
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
    // Wait for serial to open. No need for Serial.begin() because Teensy has native USB
    //while (!Serial);
    pinMode(23, OUTPUT);
    //pinMode(22, OUTPUT);
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
 
    // Set values for what is considered a TAP and what is a DOUBLE TAP (0-255)
    adxl.setTapThreshold(35);           // 62.5 mg per increment
    adxl.setTapDuration(10);            // 625 μs per increment
    adxl.setDoubleTapLatency(80);       // 1.25 ms per increment
    adxl.setDoubleTapWindow(200);       // 1.25 ms per increment
      
    // Setup Pulse Oximeter
//    spo2_sensor.begin();
//    pinMode(I2C_INT_PIN, INPUT);
//    attachInterrupt(I2C_INT_PIN, burstRead_ISR, RISING); // triggers when I2C transfer is complete

    rc = f_open(&fil, (TCHAR*)_T("test_unfiltered.bin"), FA_WRITE | FA_CREATE_ALWAYS);
    rc = f_close(&fil);
    rc = f_open(&fil, (TCHAR*)_T("test_filtered.bin"), FA_WRITE | FA_CREATE_ALWAYS);
    rc = f_close(&fil);
    
//    // Initialize Logger File
//    for(size_t i=0; ;++i) {
//        sprintf(fname, "log%u.bin", i);
//        rc = f_open(&fil, char2tchar(fname, 128, wfname), FA_WRITE | FA_CREATE_NEW);
//        if(rc == FR_OK) {
//            rc = f_close(&fil);
//            break;
//        }
//        //rc = f_close(&fil);
//    }
    //rc = f_write(&fil, magic_nums, sizeof(magic_nums), &wr);
    //rc = f_open(&fil, (TCHAR*)_T("test.bin"), FA_WRITE | FA_CREATE_ALWAYS);
    //rc = f_close(&fil);
    
    Timer3.initialize(ECG_TS);
    Timer3.attachInterrupt(ECG_TS_ISR);
}

void loop() {
    while(true) {
        sleep();
        digitalWriteFast(23, HIGH);
        noInterrupts();
        if(ecg_flag) {
          //digitalWriteFast(23, HIGH);
            ecg_buff[ecg_buff_idx++] = (float32_t)(adc->adc0->analogRead(ECG_PIN));
            ecg_flag = false;
            //digitalWriteFast(23, LOW);
        }
        if(emg_flag) {
            emg_buff[emg_buff_idx++] = (float32_t)(adc->adc0->analogRead(EMG_PIN));
            emg_flag = false;
        }
        if(adxl345_flag) {
            adxl.readAccel(&adxl345_buff[adxl345_buff_idx]);
            adxl345_buff_idx +=3;
            adxl345_flag = false;
            ADXL_ISR();//check for interrupts
        }
        if(spo2_flag) {
//            spo2_sensor.readFifoData(); // Trigger I2C Read
//            // Read value from last time
//            //uint8_t val = spo2_sensor.spO2;
//            if(spo2_sensor.spo2_ready) {
//                digitalWriteFast(23, HIGH);
//                spo2_buff[spo2_buff_idx++] = spo2_sensor.spo2;
//                spo2_sensor.spo2_ready = false;
//                digitalWriteFast(23, LOW);
//            }
            spo2_flag = false;
        }
        // Process and store collected data
        if (ecg_buff_idx%NUMSAMPLES == 0) {
            
            ecg_buff_idx = 0;
            emg_buff_idx = 0;
            adxl345_buff_idx = 0;
            spo2_buff_idx = 0;

            
            //***** Data filtering goes here *****//
            sing_trig=20;
            if (sing_trig>10){ //change to movement threshold 
              choice=1;
            ecgBP.filter(ecg_buff, ecg_buff_filt, NUMSAMPLES);
            //ecg_filter.sh_filter(ecg_buff_temp, (size_t)ECG_BUFFSIZE,  choice); 
            sing_trig=0; //reset the single trigger for movement
            }
            else if (sing_trig>5){
              choice=2;
            //emgLP.filter(emg_buff, emg_buff_filt, NUMSAMPLES);
            emgLP_70.filter(emg_buff, emg_buff_filt, NUMSAMPLES);
            emgHP_20.filter(emg_buff_filt, emg_buff_filt, NUMSAMPLES);
           // ecg_filter.sh_filter(ecg_buff, (size_t)ECG_BUFFSIZE,  choice); 
            sing_trig=0; //reset the single trigger for movement
            
            }
            //end filtering based on movement

             rc = f_open(&fil, (TCHAR*)_T("test_unfiltered.bin"), FA_WRITE | FA_OPEN_EXISTING);
            //rc = f_open(&fil, wfname, FA_WRITE | FA_OPEN_EXISTING);
            rc = f_lseek(&fil, f_size(&fil));
            
            memcpy(buffer, (uint8_t*)ecg_buff, 4*ECG_BUFFSIZE);
            memcpy(buffer+4*ECG_BUFFSIZE, (uint8_t*)emg_buff, 4*EMG_BUFFSIZE);
            memcpy(buffer+4*ECG_BUFFSIZE+4*EMG_BUFFSIZE, (uint8_t*)adxl345_buff, 2*ADXL345_BUFFSIZE);
            memcpy(buffer+4*ECG_BUFFSIZE+4*EMG_BUFFSIZE+2*ADXL345_BUFFSIZE, spo2_buff, SPO2_BUFFSIZE);
            rc = f_write(&fil, buffer, BUFFSIZE, &wr);
            rc = f_close(&fil);
            
            
            rc = f_open(&fil, (TCHAR*)_T("test_filtered.bin"), FA_WRITE | FA_OPEN_EXISTING);
            //rc = f_open(&fil, wfname, FA_WRITE | FA_OPEN_EXISTING);
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
    ++doub_trig;
  }
  
  if(adxl.triggered(interrupts, ADXL345_SINGLE_TAP)){
    ++sing_trig;
  } 
}

void ECG_TS_ISR(void) {
    //digitalWrite(23, HIGH);
    ecg_flag = true;
    adxl345_flag = true;
    emg_flag = true;
    spo2_flag = true;
    //if(!spo2_counter++%SPO2_SCALE) spo2_flag = true; // 
    //digitalWrite(23, LOW);
}
//
//void burstRead_ISR(void) { // Triggered when I2C transaction is finished on Wire
//    if(spo2_sensor.burstReadOp) {
//        uint8_t idx = 0;
//        uint8_t buffer[8];
//        while(Wire.available()) {
//            buffer[idx++] = Wire.readByte();
//        }
//        spo2_sensor.rawIRValue[spo2_sensor.raw_idx] = (buffer[0] << 8) | buffer[1];
//        spo2_sensor.rawRedValue[spo2_sensor.raw_idx++] = (buffer[2] << 8) | buffer[3];
//
//        if(!(spo2_sensor.raw_idx%4)) {
//            spo2_sensor.irACValueSqSum = 0;
//            spo2_sensor.redACValueSqSum = 0;
//            
//            // Extract AC-only components from raw data
//            for(int i=0; i<4; ++i) {
//                spo2_sensor.irACValue[i] = spo2_sensor.irDCRemover.step(spo2_sensor.rawIRValue[i]);
//                spo2_sensor.irACValueSqSum += spo2_sensor.irACValue[i]*spo2_sensor.irACValue[i];
//                spo2_sensor.redACValue[i] = spo2_sensor.redDCRemover.step(spo2_sensor.rawIRValue[i]);
//                spo2_sensor.redACValueSqSum += spo2_sensor.redACValue[i]*spo2_sensor.redACValue[i];
//            }
//
//            // Calculate SPO2 Percentage
//            float acSqRatio = 100.0 * log(spo2_sensor.redACValueSqSum/4) / log(spo2_sensor.irACValueSqSum/4);
//            uint8_t index = 0;
//            if (acSqRatio > 66)
//                index = (uint8_t)acSqRatio - 66;
//            else if (acSqRatio > 50)
//                index = (uint8_t)acSqRatio - 50;
//            spo2_sensor.spo2 = spO2LUT[index];
//            spo2_sensor.spo2_ready = true;
//            spo2_sensor.raw_idx = 0;
//        }
//    }
//}    
// ********** //
