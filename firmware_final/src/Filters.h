#ifndef FILTERS_H_INCLUDED
#define FILTERS_H_INCLUDED

#define ARM_MATH_CM4
#define __FPU_PRESENT 1
#include <arm_math.h>
#include <Arduino.h>

class iir_filter {
	public:
		iir_filter(const float32_t arr[], uint8_t stages);
		~iir_filter();
		void filter(float32_t*input, float32_t* output, uint32_t blockSize);
	private:
		uint8_t numstages;
		float32_t* coeffs;
		float32_t* state;
		arm_biquad_cascade_df2T_instance_f32 filt;
};

iir_filter::iir_filter(const float32_t arr[], uint8_t stages) : numstages(stages) {
	coeffs = new float32_t[5*numstages];
	for(int i=0; i<5*numstages; ++i)
		coeffs[i] = arr[i];
	state = new float32_t[2*numstages];
	arm_biquad_cascade_df2T_init_f32(&filt, numstages, coeffs, state);
}

iir_filter::~iir_filter() {
	delete [] coeffs;
	delete [] state;
}

void iir_filter::filter(float32_t* input, float32_t* output, uint32_t blockSize) {
	arm_biquad_cascade_df2T_f32(&filt, input, output, blockSize);
}

class fir_filter {
	public:
		fir_filter(const float32_t arr[], uint8_t tapnum, uint16_t NBlock);
		~fir_filter();
		void filter(float32_t* input, float32_t* output);
	private:
		uint8_t numTaps;
		uint16_t blockSize;
		float32_t* coeffs;
		float32_t* state;
		arm_fir_instance_f32 filt;
};

fir_filter::fir_filter(const float32_t arr[], uint8_t tapnum, uint16_t NBlock)
	: numTaps(tapnum), blockSize(NBlock) {

	coeffs = new float32_t[numTaps];
	for(int i=0; i<numTaps; ++i)
		coeffs[i] = arr[i];
	state = new float32_t[numTaps+blockSize-1];
	arm_fir_init_f32(&filt, numTaps, coeffs, state, blockSize);
}

fir_filter::~fir_filter() {
	delete [] coeffs;
	delete [] state;
}

void fir_filter::filter(float32_t* input, float32_t* output) {
	arm_fir_f32(&filt, input, output, blockSize);
}


#endif //FILTERS_H_INCLUDED
