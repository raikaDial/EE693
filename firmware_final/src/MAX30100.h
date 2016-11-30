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
#define DEFAULT_SAMPLING_RATE       MAX30100_SAMPRATE_50HZ
#define DEFAULT_PULSE_WIDTH         MAX30100_SPC_PW_1600US_16BITS
#define DEFAULT_RED_LED_CURRENT     MAX30100_LED_CURR_27_1MA
#define DEFAULT_IR_LED_CURRENT      MAX30100_LED_CURR_50MA

#define I2C_BUS_SPEED               400000UL

#define DC_REMOVER_ALPHA 95

class MAX30100 {
public:
    MAX30100(uint8_t num_var);
    ~MAX30100();
    void begin();
    void setMode(Mode mode);
    void setLedsPulseWidth(LEDPulseWidth ledPulseWidth);
    void setSamplingRate(SamplingRate samplingRate);
    void setLedsCurrent(LEDCurrent irLedCurrent, LEDCurrent redLedCurrent);
    void setHighresModeEnabled(bool enabled);

    uint8_t readRegister(uint8_t address);
    void writeRegister(uint8_t address, uint8_t data);
    void burstRead(uint8_t baseAddress, uint8_t length);
    void readFifoData();

    // Extra Variables for consolidation.
    uint8_t num_samples_var;
    uint32_t ac_idx;
    float* irACValue;
    float* redACValue;
    DCRemover irDCRemover;
    DCRemover redDCRemover;

    // SaO2 Look-up Table
    // http://www.ti.com/lit/an/slaa274b/slaa274b.pdf
    const uint8_t spO2LUT[43] = {
        100,100,100,100,99,99,99,99,99,99,98,98,98,98,
        98,97,97,97,97,97,97,96,96,96,96,96,96,95,95,
        95,95,95,95,94,94,94,94,94,93,93,93,93,93
    };

};

#endif
