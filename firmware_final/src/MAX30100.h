/*
Arduino-MAX30100 oximetry / heart rate integrated sensor library
Copyright (C) 2016  OXullo Intersecans <x@brainrapers.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MAX30100_H
#define MAX30100_H

#include <Arduino.h>
#include <stdint.h>

#include "MAX30100_Registers.h"
#include "MAX30100_Filters.h"

#define DEFAULT_MODE				MAX30100_MODE_SPO2_HR
#define DEFAULT_SAMPLING_RATE       MAX30100_SAMPRATE_200HZ
#define DEFAULT_PULSE_WIDTH         MAX30100_SPC_PW_1600US_16BITS
#define DEFAULT_RED_LED_CURRENT     MAX30100_LED_CURR_27_1MA
#define DEFAULT_IR_LED_CURRENT      MAX30100_LED_CURR_50MA

#define I2C_BUS_SPEED               400000UL
#define I2C_INT_PIN                 17

#define DC_REMOVER_ALPHA            0.95

class MAX30100 {
public:
    MAX30100();
    void begin();
    void setMode(Mode mode);
    void setLedsPulseWidth(LEDPulseWidth ledPulseWidth);
    void setSamplingRate(SamplingRate samplingRate);
    void setLedsCurrent(LEDCurrent irLedCurrent, LEDCurrent redLedCurrent);
    void setHighresModeEnabled(bool enabled);
    void update();

    volatile uint16_t rawIRValue[25];
    volatile uint16_t rawRedValue[25];
    volatile uint8_t raw_idx = 0;
    volatile uint8_t spo2;

    // Extra Stuff for Consolidation
    DCRemover irDCRemover;
    DCRemover redDCRemover;
    volatile float irACValue[25];
    volatile float irACValueSqSum;
    volatile float redACValue[25];
    volatile float redACValueSqSum;

//private:
    uint8_t readRegister(uint8_t address);
    void writeRegister(uint8_t address, uint8_t data);
    void burstRead(uint8_t baseAddress, uint8_t length);
    void readFifoData();
    volatile bool burstReadOp = false;
    volatile bool spo2_ready = false;
};

#endif
