// ***** DSP Libraries ***** //
#define ARM_MATH_CM4
#define __FPU_PRESENT 1
#include <arm_math.h>

#include "ECG_Filter.h"
ECG_Filter ecg_filter;
// ********** //

// ***** Libraries, Variables, and Helper Functions for the MicroSD Card ***** //
#include <wchar.h>
#include "ff.h"

FRESULT rc;        /* Result code */
FATFS fatfs;      /* File system object */
FIL fil_input;        /* File object */
FIL fil_output;
FIL fil_timing_peaks;
DIR dir;        /* Directory object */
FILINFO fno;      /* File information object */
UINT bw, br;
char buff[128];
TCHAR wbuff[128];

/* Stop with dying message */
void die(const char *text, FRESULT rc) {
  Serial.printf("%s: Failed with rc=%u.\r\n", text, rc);  for (;;) delay(100);
}

TCHAR * char2tchar( char * charString, size_t nn, TCHAR * tcharString) {
  for (size_t ii = 0; ii < nn; ii++) tcharString[ii] = (TCHAR) charString[ii];
  return tcharString;
}

char * tchar2char(  TCHAR * tcharString, size_t nn, char * charString){
  for(size_t ii = 0; ii<nn; ii++) charString[ii] = (char) tcharString[ii];
  return charString;
}
// ********** //

// ***** Libraries and Variables for ADXL345 ***** //
#include <SparkFun_ADXL345.h>
#define ADXL_CS_PIN 10
ADXL345 adxl = ADXL345(10);
// **********

// ***** Low Power Mode Configuration *****
//#include "Snooze.h"
//SnoozeDigital digital;
//SnoozeBlock config(digital);

void setup() {
    // Wait for serial to open. No need for Serial.begin() because Teensy has native USB
    while (!Serial);

    pinMode(23, OUTPUT);
    f_mount(&fatfs, (TCHAR*)_T("/"), 0); /* Mount/Unmount a logical drive */

    // Overwrite existing output file if it exists
    rc = f_open(&fil_output, (TCHAR*)_T("samples_filtered.csv"), FA_WRITE | FA_CREATE_ALWAYS);
    if (rc) die("Open", rc); // Exit if file failed to open
    rc = f_open(&fil_timing_peaks, (TCHAR*)_T("filter_timing_peaks.csv"), FA_WRITE | FA_CREATE_ALWAYS);
    if (rc) die("Open", rc); // Exit if file failed to open
    rc = f_open(&fil_input, (TCHAR*) _T("samples.csv"), FA_READ);
    if (rc) die("Open",rc);
    Serial.println("Opened Files!");
 
    size_t numsamples_total = 650000;
    size_t numsamples_window = 3600;
    float* ecg_data = new float[numsamples_window];
    

    // Read in all the ECG data and filter it
    char *val2_ptr;
    for(size_t i=0; i<numsamples_total; i+=numsamples_window) {
        if(i+numsamples_window > numsamples_total - 1)
            numsamples_window = numsamples_total - i;
        //f_lseek(&fil,file_idx_input);
        for (size_t j=0; j<numsamples_window; ++j) {
            if(!f_gets(wbuff, sizeof(wbuff), &fil_input)) break; // Read a line from the csv file
            tchar2char(wbuff, 128, buff); // Convert to char array
            // Search line for comma. This method assumes only two values per line in csv.
            for(size_t k=0; k<sizeof(buff); ++k) {
              if(buff[k] == ',') { 
                val2_ptr = buff + k + 1;
                break;
              }
              else if(buff[k] == '\n')
                 break;
            }
            float val2 = atof(val2_ptr);
            ecg_data[j] = val2;
        }

        uint32_t start = micros();
        uint16_t numpeaks = ecg_filter.pan85_countpeaks(ecg_data, numsamples_window);
        uint32_t end = micros();
        sprintf(buff, "%lu,%u\n", end-start, numpeaks);
        char2tchar(buff, 128, wbuff);
        bw = f_puts(wbuff, &fil_timing_peaks);
        
        for (size_t i = 0; i < numsamples_window; ++i) {
            sprintf(buff, "%f,\n", ecg_data[i]);
            char2tchar(buff, 128, wbuff);
            bw = f_puts(wbuff, &fil_output);
        }
        Serial.printf("Processed %lu of %lu\r", i+numsamples_window, numsamples_total);
    }
    
    rc = f_close(&fil_input);
    if (rc) die("Close", rc);
    rc = f_close(&fil_output);
    if (rc) die("Close", rc);
    rc = f_close(&fil_timing_peaks);
    if (rc) die("Close", rc);
    delete [] ecg_data;
    Serial.println("");
    Serial.println("All done!");
  
    // Setup ADXL345
    //digital.pinMode(9, INPUT_PULLUP, RISING); // ADXL Interrupt Pin
    adxl.setSpiBit(0); // Use 4-wire SPI
    adxl.powerOn();
    adxl.setRangeSetting(16);
    adxl.setImportantInterruptMapping(1, 1, 1, 1, 1); // Put all interrupts on pin 1
}

void loop() {
  //int who = Snooze.deepSleep(config);
  
  int x, y, z;
  adxl.readAccel(&x, &y, &z);
  ADXL_ISR();
  //Serial.printf("%d, %d, %d\n", (int16_t)x, (int16_t)y, (int16_t)z);
}

/********************* ISR *********************/
/* Look for Interrupts and Triggered Action    */
void ADXL_ISR() {
  
  // getInterruptSource clears all triggered actions after returning value
  // Do not call again until you need to recheck for triggered actions
  byte interrupts = adxl.getInterruptSource();
  
  // Free Fall Detection
  if(adxl.triggered(interrupts, ADXL345_FREE_FALL)){
    Serial.println("*** FREE FALL ***");
    //add code here to do when free fall is sensed
  } 
  
  // Inactivity
  if(adxl.triggered(interrupts, ADXL345_INACTIVITY)){
    Serial.println("*** INACTIVITY ***");
     //add code here to do when inactivity is sensed
  }
  
  // Activity
  if(adxl.triggered(interrupts, ADXL345_ACTIVITY)){
    Serial.println("*** ACTIVITY ***"); 
     //add code here to do when activity is sensed
  }
  
  // Double Tap Detection
  if(adxl.triggered(interrupts, ADXL345_DOUBLE_TAP)){
    Serial.println("*** DOUBLE TAP ***");
     //add code here to do when a 2X tap is sensed
  }
  
  // Tap Detection
  if(adxl.triggered(interrupts, ADXL345_SINGLE_TAP)){
    Serial.println("*** TAP ***");
     //add code here to do when a tap is sensed
  } 
}

