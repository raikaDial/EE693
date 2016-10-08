// ***** DSP Libraries ***** //
#define ARM_MATH_CM4
#define __FPU_PRESENT 1
#include <arm_math.h>
// ********** //

// ***** Libraries, Variables, and Helper Functions for the MicroSD Card ***** //
#include <wchar.h>
#include <ff.h>

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

  // How to open a file
  rc = f_open(&fil, (TCHAR*)_T("hello1.csv"), FA_WRITE | FA_OPEN_APPEND); // Will create if not there
  if (rc) die("Open", rc); // Exit if file failed to open

  // How to write to the file
  for (int i = 0; i < 50; ++i) {
    sprintf(buff, "%f,%f,%f\n", 0.123 * i + 1, 0.456 * i + 1, 0.789 * i + 1);
    char2tchar(buff, 128, wbuff);
    bw = f_puts(wbuff, &fil);
  }

  // How to close the file
  rc = f_close(&fil);
  if (rc) die("Close", rc);
  Serial.println("All Done!");

  // Setup ADXL345
  adxl.setSpiBit(0); // Use 4-wire SPI
  adxl.powerOn();
  adxl.setRangeSetting(16);
}

void loop() {
  int x, y, z;
  adxl.readAccel(&x, &y, &z);
  Serial.printf("%d, %d, %d\n", (int16_t)x, (int16_t)y, (int16_t)z);
  delay(50);
}
