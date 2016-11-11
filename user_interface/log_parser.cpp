#include <iostream>
#include <fstream>
#include <cstdint>

#define ECG_BUFFSIZE 1000
#define EMG_BUFFSIZE 1000
#define ADXL345_BUFFSIZE 3*1000
#define SPO2_BUFFSIZE 40
#define BUFFSIZE 2*ECG_BUFFSIZE+2*EMG_BUFFSIZE+2*ADXL345_BUFFSIZE+SPO2_BUFFSIZE

uint16_t ecg_buff[ECG_BUFFSIZE];
uint16_t emg_buff[EMG_BUFFSIZE];
int16_t adxl345_buff[ADXL345_BUFFSIZE];
uint8_t spo2_buff[SPO2_BUFFSIZE];
char buff[BUFFSIZE];
char buff1[512];

int main() {
	std::ifstream input("test.bin", std::ios::binary);
	std::ofstream ecg_out("ecg.csv");
	std::ofstream emg_out("emg.csv");
	std::ofstream adxl345_out("adxl345.csv");
	std::ofstream spo2_out("spo2.csv");

	while(!input.eof()) {
		if(input.is_open()) {
			input.read(buff, BUFFSIZE);
			if(input.eof()) break;

			// Parse the blocks of data.
			for(int i=0; i<ECG_BUFFSIZE; ++i) {
				ecg_out << std::to_string((uint16_t)buff[2*i] | ((uint16_t)(buff[2*i+1]) << 8)) << "\n";
				//ecg_buff[i] = buff[2*i] + (buff[2*i+1] << 8);
				//sprintf(buff1, "%u,", buff[2*i] + (buff[2*i+1] << 8));
				//output << buff1;
			}
			for(int i=ECG_BUFFSIZE; i<ECG_BUFFSIZE+EMG_BUFFSIZE; ++i) {
				//emg_buff[i - ECG_BUFFSIZE] = buff[2*i] + (buff[2*i+1] << 8);
				//sprintf(buff1, "%u,", buff[2*i] + (buff[2*i+1] << 8));
				//output << buff1;
				emg_out << std::to_string((uint16_t)buff[2*i] | ((uint16_t)(buff[2*i+1]) << 8)) << "\n";
			}
			for(int i=ECG_BUFFSIZE+EMG_BUFFSIZE; i<ECG_BUFFSIZE+EMG_BUFFSIZE+ADXL345_BUFFSIZE; ++i) {
				adxl345_buff[i - (ECG_BUFFSIZE+EMG_BUFFSIZE)] = (int16_t)((uint16_t)buff[2*i] | (((uint16_t)buff[2*i+1]) << 8));
				// sprintf(buff1, "%d,%d,%d,",
				// 	(int16_t)((uint16_t)buff[6*i] | (((uint16_t)buff[6*i+1]) << 8)),
				// 	(int16_t)((uint16_t)buff[6*i+2] | (((uint16_t)buff[6*i+3]) << 8)),
				// 	(int16_t)((uint16_t)buff[6*i+4] | (((uint16_t)buff[6*i+5]) << 8))
				// );
				//output << buff1;
			}
			for(int i=ECG_BUFFSIZE+EMG_BUFFSIZE+ADXL345_BUFFSIZE; i<ECG_BUFFSIZE+EMG_BUFFSIZE+ADXL345_BUFFSIZE+SPO2_BUFFSIZE; ++i) {
				//spo2_buff[i - (ECG_BUFFSIZE+EMG_BUFFSIZE+ADXL345_BUFFSIZE)] = buff[i];
				//sprintf(buff1, "%u\n", (uint16_t)buff[i]);
				spo2_out << std::to_string(buff[i]) << "\n";
			}
			for(int i=0; i<ADXL345_BUFFSIZE; i+=3) {
				adxl345_out << std::to_string(adxl345_buff[i]) << "," << std::to_string(adxl345_buff[i+1]) << "," << std::to_string(adxl345_buff[i+2]) << "\n";
			}
		}
	}
}