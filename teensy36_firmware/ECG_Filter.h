#ifndef ECG_FILTER_H
#define ECG_FILTER_H

#include "Arduino.h"

class ECG_Filter {
    public:
        uint16_t pan85_countpeaks(float* signal, size_t length);
        void pan85_filter(float* signal, size_t length);
        
        void pan85_filter(uint16_t* signal, size_t length);
        uint16_t pan85_countpeaks(uint16_t* signal, size_t length);
    private:
        bool transient_included = true;
        void pan85_lp(float* signal, float* output, size_t length);
        void pan85_hp(float* signal, float* output, size_t length);
        void pan85_dev(float* signal, float* output, size_t length);

        void pan85_lp(uint16_t* signal, uint16_t* output, size_t length);
        void pan85_hp(uint16_t* signal, uint16_t* output, size_t length);
        void pan85_dev(uint16_t* signal, uint16_t* output, size_t length);
        
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
        template <class type>
        type maximum(type* arr, const size_t length, size_t & idx) {
            type max_val = 0;
            idx = 0;
            for(size_t i=0; i < length; ++i) {
                if(arr[i] >  max_val) {
                    max_val = arr[i];
                    idx = i;
                }
            }
            return max_val;
        }
        template <class type>
        type mean(type* arr, const size_t length) {
            type avg  = 0;
            for(int i=0; i<length; ++i)
                avg += arr[i];
            return avg/=length;
        }
};

#endif //ECG_FILTER_H

