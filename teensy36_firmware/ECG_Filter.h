#ifndef ECG_FILTER_H
#define ECG_FILTER_H

#include "Arduino.h"

class ECG_Filter {
    public:
        uint16_t pan85_countpeaks(float* signal, size_t length);
        void pan85_filter(float* signal, size_t length);
        
        void pan85_filter(int32_t* signal, size_t length);
        uint16_t pan85_countpeaks(int32_t* signal, size_t length);
    private:
        void pan85_lp(float* signal, float* output, size_t length);
        void pan85_hp(float* signal, float* output, size_t length);
        void pan85_dev(float* signal, float* output, size_t length);

        void pan85_lp(int32_t* signal, int32_t* output, size_t length);
        void pan85_hp(int32_t* signal, int32_t* output, size_t length);
        void pan85_dev(int32_t* signal, int32_t* output, size_t length);
        
        template <class type> 
        void square_signal(type* signal, type* output, size_t length) {
            for(int i=0; i<length; ++i)
                output[i] = signal[i]*signal[i];
        }
        template <class type> 
        void copy_signal(type* source, type* dest, size_t length) {
            for(int i=0; i<length; ++i)
                dest[i] = source[i];
        }
};

#endif //ECG_FILTER_H

