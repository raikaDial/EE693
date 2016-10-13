#include "ECG_Filter.h"

void ECG_Filter::pan85_filter(float* signal, uint16_t length) {
    if(length > 32) {
        pan85_lp(signal,length);
        pan85_hp(signal,length);
        pan85_dev(signal,length);
        square_signal(signal,length);
    }
}

void ECG_Filter::pan85_lp(float* signal, uint16_t length) {
    float* output = new float[length];

    // Initial Conditions
    output[0] = signal[0];
    output[1] = signal[1];
    
    for(int n=2; n<6; ++n)
        output[n] = 2*output[n-1] - output[n-2] + signal[n];
    for(int n=6; n<12; ++n)
        output[n] = 2*output[n-1] - output[n-2] + signal[n] - 2*signal[n-6];
    for(int n=12; n<length; ++n)
        output[n] = 2*output[n-1] - output[n-2] + signal[n] - 2*signal[n-6] + signal[n-12];

    copy_signal(output, signal, length);
    delete [] output;
}

void ECG_Filter::pan85_hp(float* signal, uint16_t length) {
    float* output = new float[length];

    // Initial Condition
    output[0] = -signal[0]/32;
    
    for(uint16_t n=1; n<16; ++n)
      output[n] = output[n-1] - signal[n]/32;
    output[16] = output[15] - signal[16]/32 + signal[0];
    for(uint16_t n = 17; n < 32; ++n)
        output[n] = output[n-1] - signal[n]/32 + signal[n-16] - signal[n-17];
    for(uint16_t n = 32; n < length; ++n)
        output[n] = output[n-1] - signal[n]/32 + signal[n-16] - signal[n-17] + signal[n-32]/32;

    copy_signal(output, signal, length);
    delete [] output;
}

void ECG_Filter::pan85_dev(float* signal, uint16_t length) {
    float* output = new float[length];

    // Initial Conditions
    output[0] = 2*signal[0]/8;
    output[1] = (2*signal[1] + signal[0])/8;
    output[2] = (2*signal[2] + signal[1] - signal[0])/8;
    output[3] = (2*signal[3] + signal[2] - signal[1] - 2*signal[0])/8;
    
    for(uint16_t n=4; n<length; ++n)
        output[n] = (2*signal[n] + signal[n-1] - signal[n-2] - 2*signal[n-3])*0.1; // 1/8 approx for ints

    copy_signal(output, signal, length);
    delete [] output;
}

void ECG_Filter::square_signal(float* signal, uint16_t length) {
    for(int i=0; i<length; ++i)
        signal[i] = signal[i]*signal[i];
}

void ECG_Filter::copy_signal(float* source, float *dest, uint16_t length) {
    for(int i=0; i<length; ++i)
        dest[i] = source[i];
}

