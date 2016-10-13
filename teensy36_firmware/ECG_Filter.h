#ifndef ECG_FILTER_H
#define ECG_FILTER_H

#include "Arduino.h"

class ECG_Filter {
    public:
        void pan85_filter(float* signal, uint16_t length);
    private:
        void pan85_lp(float* signal, uint16_t length);
        void pan85_hp(float* signal, uint16_t length);
        void pan85_dev(float* signal, uint16_t length);
        void square_signal(float* signal, uint16_t length);
        void copy_signal(float* source, float *dest, uint16_t length);
};

#endif //ECG_FILTER_H

