#include "ECG_Filter.h"

void ECG_Filter::pan85_filter(float* signal, size_t length) {
    if(length > 32) {
        pan85_lp(signal,length);
        pan85_hp(signal,length);
        pan85_dev(signal,length);
        square_signal(signal,length);
    }
}

void ECG_Filter::pan85_lp(float* signal, size_t length) {
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

void ECG_Filter::pan85_hp(float* signal, size_t length) {
    float* output = new float[length];

    // Initial Condition
    output[0] = -signal[0]/32;
    
    for(size_t n=1; n<16; ++n)
      output[n] = output[n-1] - signal[n]/32;
    output[16] = output[15] - signal[16]/32 + signal[0];
    for(size_t n = 17; n < 32; ++n)
        output[n] = output[n-1] - signal[n]/32 + signal[n-16] - signal[n-17];
    for(size_t n = 32; n < length; ++n)
        output[n] = output[n-1] - signal[n]/32 + signal[n-16] - signal[n-17] + signal[n-32]/32;

    copy_signal(output, signal, length);
    delete [] output;
}

void ECG_Filter::pan85_dev(float* signal, size_t length) {
    float* output = new float[length];

    // Initial Conditions
    output[0] = 2*signal[0]/8;
    output[1] = (2*signal[1] + signal[0])*0.1;
    output[2] = (2*signal[2] + signal[1] - signal[0])*0.1;
    output[3] = (2*signal[3] + signal[2] - signal[1] - 2*signal[0])*0.1;
    
    for(size_t n=4; n<length; ++n)
        output[n] = (2*signal[n] + signal[n-1] - signal[n-2] - 2*signal[n-3])*0.1;

    copy_signal(output, signal, length);
    delete [] output;
}


// ***** Optimized for fixed precision or integer data ***** //

void ECG_Filter::pan85_filter(int32_t* signal, size_t length) {
    if(length > 32) {
        pan85_lp(signal,length);
        pan85_hp(signal,length);
        pan85_dev(signal,length);
        square_signal(signal,length);
    }
}

void ECG_Filter::pan85_lp(int32_t* signal, size_t length) {
    int32_t* output = new int32_t[length];

    // Initial Conditions
    output[0] = signal[0];
    output[1] = signal[1];
    
    for(int n=2; n<6; ++n)
        output[n] = (output[n-1] << 1) - output[n-2] + signal[n];
    for(int n=6; n<12; ++n)
        output[n] = (output[n-1] << 1) - output[n-2] + signal[n] - (signal[n-6] << 1);
    for(int n=12; n<length; ++n)
        output[n] = (output[n-1] << 1) - output[n-2] + signal[n] - (signal[n-6] << 1) + signal[n-12];

    copy_signal(output, signal, length);
    delete [] output;
}

void ECG_Filter::pan85_hp(int32_t* signal, size_t length) {
    int32_t* output = new int32_t[length];

    // Initial Condition
    output[0] = -signal[0]/32;
    
    for(size_t n=1; n<16; ++n)
      output[n] = output[n-1] - (signal[n] >> 5);
    output[16] = output[15] - (signal[16] >> 5) + signal[0];
    for(size_t n = 17; n < 32; ++n)
        output[n] = output[n-1] - (signal[n] >> 5) + signal[n-16] - signal[n-17];
    for(size_t n = 32; n < length; ++n)
        output[n] = output[n-1] - (signal[n] >> 5) + signal[n-16] - signal[n-17] + (signal[n-32] >> 5);

    copy_signal(output, signal, length);
    delete [] output;
}

void ECG_Filter::pan85_dev(int32_t* signal, size_t length) {
    int32_t* output = new int32_t[length];

    // Initial Conditions
    output[0] = signal[0] >> 2;
    output[1] = ((signal[1] << 1) + signal[0]) >> 3;
    output[2] = ((signal[2] << 1) + signal[1] - signal[0]) >> 3;
    output[3] = ((signal[3] << 1) + signal[2] - signal[1] - (signal[0] << 1)) >> 3;
    
    for(size_t n=4; n<length; ++n)
        output[n] = ((signal[n] << 1) + signal[n-1] - signal[n-2] - (signal[n-3] << 1)) >> 3; // 1/8 approx for ints

    copy_signal(output, signal, length);
    delete [] output;
}
