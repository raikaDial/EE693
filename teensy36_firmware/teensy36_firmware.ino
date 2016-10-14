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
FIL fil;        /* File object */
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

void setup() {
    // Wait for serial to open. No need for Serial.begin() because Teensy has native USB
    while (!Serial);
  
    f_mount(&fatfs, (TCHAR*)_T("/"), 0); /* Mount/Unmount a logical drive */

    // Overwrite existing output file if it exists
    rc = f_open(&fil, (TCHAR*)_T("samples_filtered.csv"), FA_WRITE | FA_CREATE_ALWAYS);
    if (rc) die("Open", rc); // Exit if file failed to open
    rc = f_close(&fil);
    if (rc) die("Close", rc);
  
    size_t numsamples_total = 21600;
    size_t numsamples_window = 50;
    float* ecg_data = new float[numsamples_window];

    // Read in all the ECG data and filter it
    FSIZE_t file_idx_input = 0;
    FSIZE_t file_idx_output = 0;
    char *val2_ptr;
    for(size_t i=0; i<numsamples_total; i+=numsamples_window) {
        rc = f_open(&fil, (TCHAR*) _T("samples.csv"), FA_READ);
        if (rc) die("Open",rc);
        f_lseek(&fil,file_idx_input);
        for (size_t j=0; j<numsamples_window; ++j) {
            if(!f_gets(wbuff, sizeof(wbuff), &fil)) break; // Read a line from the csv file
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
        file_idx_input = f_tell(&fil);
        rc = f_close(&fil);
        if (rc) die("Close", rc);

        ecg_filter.pan85_filter(ecg_data, numsamples_window);
        rc = f_open(&fil, (TCHAR*)_T("samples_filtered.csv"), FA_WRITE | FA_OPEN_EXISTING);
        if (rc) die("Open", rc); // Exit if file failed to open
        f_lseek(&fil,file_idx_output);
        
        for (int i = 0; i < numsamples_window; ++i) {
            sprintf(buff, "%f,\n", ecg_data[i]);
            char2tchar(buff, 128, wbuff);
            bw = f_puts(wbuff, &fil);
        }
        file_idx_output = f_tell(&fil);
        rc = f_close(&fil);
        if (rc) die("Close", rc);
    }
    delete [] ecg_data;
    Serial.println("All done!");
  
    // Setup ADXL345
    adxl.setSpiBit(0); // Use 4-wire SPI
    adxl.powerOn();
    adxl.setRangeSetting(16);
}

void loop() {
  int x, y, z;
  adxl.readAccel(&x, &y, &z);
  //Serial.printf("%d, %d, %d\n", (int16_t)x, (int16_t)y, (int16_t)z);
  delay(50);
}
