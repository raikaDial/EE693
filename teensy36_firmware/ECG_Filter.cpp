#include "ECG_Filter.h"
#include <arm_math.h>

uint16_t ECG_Filter::pan85_countpeaks(float* signal, size_t length) {
    uint16_t numpeaks = 1;
    pan85_filter(signal, length);
  
    size_t peak_pos = 0;
    float peak_val = maximum(signal+50, 50, peak_pos);
    // Initial Thresholds
    float thr_high = 0.70*peak_val;
    float thr_low = 0.3*peak_val;
    static const size_t min_RR = 108;
    size_t RR[10]; // Store last 10 RR intervals
    size_t RR_numvals = 0;
    for(size_t i=0; i<length; ++i) {
        static size_t dummy; // dummy variable for maximum function
        if( (signal[i] >= thr_high) && (i > peak_pos + min_RR) ) {
            ++numpeaks;
            RR[numpeaks%10] = i - peak_pos;
            RR_numvals < 10 ? ++RR_numvals : RR_numvals;
            peak_pos = i;
            
            if(i < length - 50)
                peak_val = maximum(signal+i, 50, dummy);
            else
                peak_val = maximum(signal+i, length-i, dummy);
  
            // Update thresholds
            thr_high = 0.70*peak_val;
            thr_low = 0.3*peak_val;
        }
        // If there has not been a detection in a while, go back and search using the lower threshold
        else if( (i-peak_pos) > 2*mean(RR, RR_numvals) ) {
            for(size_t j=peak_pos+50; j < i; ++j) {
                if( (j > (peak_pos + min_RR)) && (signal[j] >= thr_low) ) {
                    ++numpeaks;
                    RR[numpeaks%10] = j - peak_pos;
                    RR_numvals < 10 ? ++RR_numvals : RR_numvals;
                    peak_pos = j;
                    if(j < length - 50)
                        peak_val = maximum(signal+j, 50, dummy);
                    else
                        peak_val = maximum(signal+j, length-j, dummy);
  
                    // Update thresholds
                    thr_high = 0.70*peak_val;
                    thr_low = 0.3*peak_val;
                }
            }
        }
    } 
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
        if(transient_included) {
            for(size_t i=0; i<50; ++i)
                signal[i] = 0;
            transient_included = false;
        }
        delete [] buff;
    }
}

void ECG_Filter::pan85_lp(float* signal, float* output, size_t length) {
    static float past_inputs[12] = {};
    static float past_outputs[2] = {};

    // Initial Conditions
    output[0] = 2*past_outputs[1] - past_outputs[0] + signal[0] - 2*past_inputs[6] + past_inputs[0];
    output[1] = 2*output[0] - past_outputs[1] + signal[1] - 2*past_inputs[7] + past_inputs[1];
    
    for(int n=2; n<6; ++n)
        output[n] = 2*output[n-1] - output[n-2] + signal[n] - 2*past_inputs[n+6] + past_inputs[n];
    for(int n=6; n<12; ++n)
        output[n] = 2*output[n-1] - output[n-2] + signal[n] - 2*signal[n-6] + past_inputs[n];
    for(size_t n=12; n<length; ++n)
        output[n] = 2*output[n-1] - output[n-2] + signal[n] - 2*signal[n-6] + signal[n-12];
    for(int n=0; n<12; ++n)
        past_inputs[n] = signal[(length-1) - 11 + n];
    past_outputs[0] = output[(length-1)-1];
    past_outputs[1] = output[(length-1)];
}

void ECG_Filter::pan85_hp(float* signal, float* output, size_t length) {
    static float past_inputs[32] = {};
    static float past_output = 0;
    // Initial Condition
    output[0] = past_output - signal[0]/32 + past_inputs[16] - past_inputs[15] + past_inputs[0]/32;
    
    for(int n=1; n<16; ++n)
      output[n] = output[n-1] - signal[n]/32 + past_inputs[n+16] - past_inputs[n+15] + past_inputs[n]/32;
    output[16] = output[15] - signal[16]/32 + signal[0] - past_inputs[31] + past_inputs[16]/32;
    for(int n = 17; n < 32; ++n)
        output[n] = output[n-1] - signal[n]/32 + signal[n-16] - signal[n-17] + past_inputs[n]/32;
    for(size_t n = 32; n < length; ++n)
        output[n] = output[n-1] - signal[n]/32 + signal[n-16] - signal[n-17] + signal[n-32]/32;

    for(int n=0; n<32; ++n)
        past_inputs[n] = signal[(length-1) - 31 + n];
    past_output = output[(length - 1)];
}

void ECG_Filter::pan85_dev(float* signal, float* output, size_t length) {
    static float past_inputs[4] = {};
    // Initial Conditions
    output[0] = (2*signal[0] + past_inputs[3] - past_inputs[1] - 2*past_inputs[0])*0.1;
    output[1] = (2*signal[1] + signal[0] - past_inputs[2] - 2*past_inputs[1])*0.1;
    output[2] = (2*signal[2] + signal[1] - past_inputs[3] - 2*past_inputs[2])*0.1;
    output[3] = (2*signal[3] + signal[2] - signal[0] - 2*past_inputs[3])*0.1;
    
    for(size_t n=4; n<length; ++n)
        output[n] = (2*signal[n] + signal[n-1] - signal[n-3] - 2*signal[n-4])*0.1;
    for(int n=0; n<4; ++n)
        past_inputs[n] = signal[(length-1) - 3 + n];
}


// ***** Optimized for fixed precision or integer data ***** //

uint16_t ECG_Filter::pan85_countpeaks(uint16_t* signal, size_t length) {
  uint16_t numpeaks = 0;
  pan85_filter(signal, length);
  
  return numpeaks;
}

void ECG_Filter::pan85_filter(uint16_t* signal, size_t length) {
    if(length > 128) {
        uint16_t* buff = new uint16_t[length];
        pan85_lp(signal,buff,length);
        pan85_hp(buff,signal,length);
        pan85_dev(signal,buff,length);
        square_signal(buff,signal,length);
        // Chop off first 50 samples to remove filter edge effects
        if(transient_included) {
            for(size_t i=0; i<50; ++i)
                signal[i] = 0;
            transient_included = false;
        }
        delete [] buff;
    }
}

void ECG_Filter::pan85_lp(uint16_t* signal, uint16_t* output, size_t length) {
    static uint16_t past_inputs[12] = {};
    static uint16_t past_outputs[2] = {};

    // Initial Conditions
    output[0] = (past_outputs[1] << 1)- past_outputs[0] + signal[0] - (past_inputs[6] << 1) + past_inputs[0];
    output[1] = (output[0] << 1) - past_outputs[1] + signal[1] - (past_inputs[7] << 1) + past_inputs[1];
    
    for(int n=2; n<6; ++n)
        output[n] = (output[n-1] << 1) - output[n-2] + signal[n] - (past_inputs[n+6] << 1) + past_inputs[n];
    for(int n=6; n<12; ++n)
        output[n] = (output[n-1] << 1) - output[n-2] + signal[n] - (signal[n-6] << 1) + past_inputs[n];
    for(size_t n=12; n<length; ++n)
        output[n] = (output[n-1] << 1) - output[n-2] + signal[n] - (signal[n-6] << 1) + signal[n-12];
    for(int n=0; n<12; ++n)
        past_inputs[n] = signal[(length-1) - 11 + n];
    past_outputs[0] = output[(length-1)-1];
    past_outputs[1] = output[(length-1)];
}

void ECG_Filter::pan85_hp(uint16_t* signal, uint16_t* output, size_t length) {
    static uint16_t past_inputs[32] = {};
    static uint16_t past_output = 0;
    // Initial Condition
    output[0] = past_output - (signal[0] >> 5) + past_inputs[16] - past_inputs[15] + (past_inputs[0] >> 5);
    
    for(int n=1; n<16; ++n)
      output[n] = output[n-1] - (signal[n] >> 5) + past_inputs[n+16] - past_inputs[n+15] + (past_inputs[n] >> 5);
    output[16] = output[15] - (signal[16] >> 5) + signal[0] - past_inputs[31] + (past_inputs[16] >> 5);
    for(int n = 17; n < 32; ++n)
        output[n] = output[n-1] - (signal[n] >> 5) + signal[n-16] - signal[n-17] + (past_inputs[n] >> 5);
    for(size_t n = 32; n < length; ++n)
        output[n] = output[n-1] - (signal[n] >> 5) + signal[n-16] - signal[n-17] + (signal[n-32] >> 5);

    for(int n=0; n<32; ++n)
        past_inputs[n] = signal[(length-1) - 31 + n];
    past_output = output[(length - 1)];
}

void ECG_Filter::pan85_dev(uint16_t* signal, uint16_t* output, size_t length) {
    static uint16_t past_inputs[4] = {};
    // Initial Conditions
    output[0] = ( (signal[0] << 1) + past_inputs[3] - past_inputs[1] - (past_inputs[0] << 1) ) >> 3;
    output[1] = ( (signal[1] << 1) + signal[0] - past_inputs[2] - (past_inputs[1] << 1) ) >> 3;
    output[2] = ( (signal[2] << 1) + signal[1] - past_inputs[3] - (past_inputs[2] << 1) ) >> 3;
    output[3] = ( (signal[3] << 1) + signal[2] - signal[0] - (past_inputs[3] << 1) ) >> 3;
    
    for(size_t n=4; n<length; ++n)
        output[n] = ( (signal[n] << 1) + signal[n-1] - signal[n-3] - (signal[n-4] << 1) ) >> 3;
    for(int n=0; n<4; ++n)
        past_inputs[n] = signal[(length-1) - 3 + n];
}




void ECG_Filter::sh_filter(float32_t* signal, size_t length, size_t choice){
if(length > 128) {
       float32_t* buff = new float32_t[length];
        switch (choice){
          case 1:
        IIR_but_70_lp(signal,buff,length); //emg high and low pass filters
        IIR_but_20_hp(buff,signal,length); 
          break;
          case 2:
        IIR_but_70_lp(signal,buff,length); //emg high and low pass filters
        IIR_but_20_hp(buff,signal,length);  
          break;
          case 3:
          //do something here 
          break;
          
          default: 
          //do standard filters here 
          break;
        }
        if(transient_included) {
            for(size_t i=0; i<50; ++i)
                signal[i] = 0;
            transient_included = false;
        }
        delete [] buff;
    }
}


//lp filter at 70 to be applied to EMG for noise removal along with high pass at 20hz
void ECG_Filter::IIR_but_70_lp(float32_t* signal, float32_t* output, size_t length) {//2nd order lp filter 70 hz IIR
    static int16_t past_inputs[2] = {};
  static int16_t past_outputs[2] = {};
    // Initial Conditions
    output[0] = 0.3503*signal[0] + 0.7007*past_inputs[1] + 0.3503*past_inputs[0]+ 0.2212*past_outputs[1] + 0.1802*past_outputs[0];
    output[1] = 0.3503*signal[1] + 0.7007*signal[0]      + 0.3503*past_inputs[1]+ 0.2212*output[0]     + 0.1802*past_outputs[1];
  
    for(int n=2; n<length; ++n)
    output[n] = 0.3503*signal[n] + 0.7007*signal[n-1]    + 0.3503*signal[n-2]   + 0.2212*output[n-1]    + 0.1802*output[n-2];
  
    for(size_t n=0; n<2; ++n)
        past_inputs[n] = signal[(length-1) - 1+ n];
  
  past_outputs[0]=output[(length-1)-1];
  past_outputs[1]=output[(length-1)];
}

void ECG_Filter::IIR_but_20_hp(float32_t* signal, float32_t* output, size_t length) { //2nd order IIR butterworth filter 20hz hp
  static int16_t past_inputs[2] = {};
  static int16_t past_outputs[2] = {};
    // Initial Conditions
    output[0] = 0.6998*signal[0] - 1.3995*past_inputs[1] + 0.6998*past_inputs[0]  - 1.3073*past_outputs[1] + 0.4918*past_outputs[0];
    output[1] = 0.6998*signal[1] - 1.3995*signal[0]      + 0.6998*past_inputs[1]  - 1.3073*output[0]     + 0.4918*past_outputs[1];
  
    for(int n=2; n<length; ++n)
    output[n] = 0.6998*signal[n] - 1.3995*signal[n-1]    + 0.6998*signal[n-2]     - 1.3073*output[n-1]    + 0.4918*output[n-2];
  
    for(size_t n=0; n<2; ++n)
        past_inputs[n] = signal[(length-1) - 1+ n];
  
  past_outputs[0]=output[(length-1)-1];
  past_outputs[1]=output[(length-1)];

}


