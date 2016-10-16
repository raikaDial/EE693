#include "ECG_Filter.h"

uint16_t ECG_Filter::pan85_countpeaks(float* signal, size_t length) {
  uint16_t numpeaks = 0;
  pan85_filter(signal, length);
  
  return numpeaks;
}

void ECG_Filter::pan85_filter(float* signal, size_t length) {
    if(length > 128) {
        float* buff = new float[length];
        pan85_lp(signal,buff,length);
        pan85_hp(buff,signal,length);
        pan85_dev(signal,buff,length);
        square_signal(buff,signal,length);
        // Chop off first 50 samples to remove filter edge effects
        for(size_t i=0; i<50; ++i)
            signal[i] = 0;
        delete [] buff;
    }
}

void ECG_Filter::pan85_lp(float* signal, float* output, size_t length) {
    // Initial Conditions
    output[0] = signal[0];
    output[1] = signal[1];
    
    for(int n=2; n<6; ++n)
        output[n] = 2*output[n-1] - output[n-2] + signal[n];
    for(int n=6; n<12; ++n)
        output[n] = 2*output[n-1] - output[n-2] + signal[n] - 2*signal[n-6];
    for(size_t n=12; n<length; ++n)
        output[n] = 2*output[n-1] - output[n-2] + signal[n] - 2*signal[n-6] + signal[n-12];
}

void ECG_Filter::pan85_hp(float* signal, float* output, size_t length) {
    // Initial Condition
    output[0] = -signal[0]/32;
    
    for(int n=1; n<16; ++n)
      output[n] = output[n-1] - signal[n]/32;
    output[16] = output[15] - signal[16]/32 + signal[0];
    for(int n = 17; n < 32; ++n)
        output[n] = output[n-1] - signal[n]/32 + signal[n-16] - signal[n-17];
    for(size_t n = 32; n < length; ++n)
        output[n] = output[n-1] - signal[n]/32 + signal[n-16] - signal[n-17] + signal[n-32]/32;
}

void ECG_Filter::pan85_dev(float* signal, float* output, size_t length) {
    // Initial Conditions
    output[0] = 2*signal[0]*0.1;
    output[1] = (2*signal[1] + signal[0])*0.1;
    output[2] = (2*signal[2] + signal[1])*0.1;
    output[3] = (2*signal[3] + signal[2] - signal[0])*0.1;
    output[4] = (2*signal[4] + signal[3] - signal[1] - 2*signal[0])*0.1;
    
    for(size_t n=5; n<length; ++n)
        output[n] = (2*signal[n] + signal[n-1] - signal[n-3] - 2*signal[n-4])*0.1;
}


// ***** Optimized for fixed precision or integer data ***** //

uint16_t ECG_Filter::pan85_countpeaks(int32_t* signal, size_t length) {
  uint16_t numpeaks = 0;
  pan85_filter(signal, length);
  
  return numpeaks;
}

void ECG_Filter::pan85_filter(int32_t* signal, size_t length) {
    if(length > 128) {
        int32_t* buff = new int32_t[length];
        pan85_lp(signal,buff,length);
        pan85_hp(buff,signal,length);
        pan85_dev(signal,buff,length);
        square_signal(buff,signal,length);
        // Chop off first 50 samples to remove filter edge effects
        for(size_t i=0; i<50; ++i)
            signal[i] = 0;
        delete [] buff;
    }
}

void ECG_Filter::pan85_lp(int32_t* signal, int32_t* output, size_t length) {
    // Initial Conditions
    output[0] = signal[0];
    output[1] = signal[1];
    
    for(int n=2; n<6; ++n)
        output[n] = (output[n-1] << 1) - output[n-2] + signal[n];
    for(int n=6; n<12; ++n)
        output[n] = (output[n-1] << 1) - output[n-2] + signal[n] - (signal[n-6] << 1);
    for(int n=12; n<length; ++n)
        output[n] = (output[n-1] << 1) - output[n-2] + signal[n] - (signal[n-6] << 1) + signal[n-12];
}

void ECG_Filter::pan85_hp(int32_t* signal, int32_t* output, size_t length) {
    // Initial Condition
    output[0] = -signal[0]/32;
    
    for(size_t n=1; n<16; ++n)
      output[n] = output[n-1] - (signal[n] >> 5);
    output[16] = output[15] - (signal[16] >> 5) + signal[0];
    for(size_t n = 17; n < 32; ++n)
        output[n] = output[n-1] - (signal[n] >> 5) + signal[n-16] - signal[n-17];
    for(size_t n = 32; n < length; ++n)
        output[n] = output[n-1] - (signal[n] >> 5) + signal[n-16] - signal[n-17] + (signal[n-32] >> 5);
}

void ECG_Filter::pan85_dev(int32_t* signal, int32_t* output, size_t length) {
    // Initial Conditions
    output[0] = signal[0] >> 2;
    output[1] = ((signal[1] << 1) + signal[0]) >> 3;
    output[2] = ((signal[2] << 1) + signal[1] - signal[0]) >> 3;
    output[3] = ((signal[3] << 1) + signal[2] - signal[0]) >> 3;
    output[4] = ((signal[4] << 1) + signal[3] - signal[1] - (signal[0] << 1)) >> 3;
    
    for(size_t n=5; n<length; ++n)
        output[n] = ((signal[n] << 1) + signal[n-1] - signal[n-2] - (signal[n-3] << 1)) >> 3; // 1/8 approx for ints
}
