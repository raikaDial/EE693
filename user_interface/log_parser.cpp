#include <iostream>
#include <fstream>
#include <cstdint>

uint16_t ecg_buff[1000];
uint16_t emg_buff[1000];
int16_t adxl_buff[3*1000];
uint8_t spo2_buff[40];
char buff[10040];

int main() {
	std::ifstream file("test.bin", std::ios::binary);
	if(file.is_open()) {
		file.read(buff, 10040);
		int ecg_idx = 0;
		for(int i=0; i<1000; ++i) {
			ecg_buff[i] = buff[2*i] + (buff[2*i+1] << 8);
		}
		for(int i=1000; i<2000; ++i) {
			emg_buff[i-1000] = buff[2*i] + (buff[2*i+1] << 8);
		}
		for(int i=2000; i<5000; ++i) {
			adxl_buff[i-2000] = (int16_t)((uint16_t)buff[2*i] | (((uint16_t)buff[2*i+1]) << 8));
		}
		for(int i=10000; i<10040; ++i) {
			spo2_buff[i] = buff[i];
		}
		for(int i=0; i<1000; ++i) {
			printf("%d, %d, %d\n", adxl_buff[3*i], adxl_buff[3*i+1], adxl_buff[3*i+2]);
			//std::cout << adxl_buff[3*i] << " " << adxl_buff[3*i+1] << adxl_buff[3*i+2] << std::endl;
		}
	}
}