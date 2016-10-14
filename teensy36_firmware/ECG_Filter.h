#ifndef ECG_FILTER_H
#define ECG_FILTER_H

#include "Arduino.h"

class ECG_Filter {
    public:
        void pan85_filter(float* signal, uint16_t length);
        void pan85_filter(int32_t* signal, uint16_t length);
    private:
        void pan85_lp(float* signal, uint16_t length);
        void pan85_hp(float* signal, uint16_t length);
        void pan85_dev(float* signal, uint16_t length);

        template <class type> 
        void square_signal(type* signal, uint16_t length) {
            for(int i=0; i<length; ++i)
                signal[i] = signal[i]*signal[i];
        }
        template <class type> 
        void copy_signal(type* source, type* dest, uint16_t length) {
            for(int i=0; i<length; ++i)
                dest[i] = source[i];
        }
//        void square_signal(float* signal, uint16_t length);
//        void copy_signal(float* source, float* dest, uint16_t length);

        void pan85_lp(int32_t* signal, uint16_t length);
        void pan85_hp(int32_t* signal, uint16_t length);
        void pan85_dev(int32_t* signal, uint16_t length);
//        void square_signal(int32_t* signal, uint16_t length);
//        void copy_signal(int32_t* source, int32_t* dest, uint16_t length);
};

#endif //ECG_FILTER_H

