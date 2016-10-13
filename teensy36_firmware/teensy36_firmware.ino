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
char *val2_ptr;
int comma_idx = -1;

/* Stop with dying message */
void die(const char *text, FRESULT rc) {
  Serial.printf("%s: Failed with rc=%u.\r\n", text, rc);  for (;;) delay(100);
}

TCHAR * char2tchar( char * charString, size_t nn, TCHAR * tcharString) {
  for (size_t ii = 0; ii < nn; ii++) tcharString[ii] = (TCHAR) charString[ii];
  return tcharString;
}

char * tchar2char(  TCHAR * tcharString, size_t nn, char * charString)
{ int ii;
  for(ii = 0; ii<nn; ii++) charString[ii] = (char) tcharString[ii];
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

  rc = f_open(&fil, (TCHAR*) _T("samples.txt"), FA_READ);
  if (rc) die("Open",rc);

  Serial.println("Get the file content.");
  float ecg_data[2048];
  uint16_t length = 2048;
  for (int i=0; i<length; ++i) 
  {
      if(!f_gets(wbuff, sizeof(wbuff), &fil)) break; // Read a line from the csv file
      tchar2char(wbuff, 128, buff); // Convert to char array
      // Search line for comma. This method assumes only two values per line in csv.
      for(int i=0; i<sizeof(buff); ++i) {
        if(buff[i] == ',') { 
          val2_ptr = buff + i + 1;
          break;
        }
        else if(buff[i] == '\n')
           break;
      }
      //float val1 = atof(buff);
      float val2 = atof(val2_ptr);
      ecg_data[i] = val2;
//      Serial.printf("%f\n",val1);
      Serial.printf("%f\n",val2);
  }
  rc = f_close(&fil);
  if (rc) die("Close", rc);
  ecg_filter.pan85_filter(ecg_data, length);
  rc = f_open(&fil, (TCHAR*)_T("samples_filtered.csv"), FA_WRITE | FA_CREATE_ALWAYS);
  if (rc) die("Open", rc); // Exit if file failed to open
  for (int i = 0; i < length; ++i) {
    sprintf(buff, "%f,\n", ecg_data[i]);
    char2tchar(buff, 128, wbuff);
    bw = f_puts(wbuff, &fil);
  }

//  // How to close the file
  rc = f_close(&fil);
  if (rc) die("Close", rc);
  Serial.println("All done!");
//  Serial.println("All Done!");

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
