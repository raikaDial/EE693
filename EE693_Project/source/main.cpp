#include "sys_common.h"
#include "system.h"
#include "gio.h"
#include "sci.h"
#include <string>
#include <iostream>
#include "stdio.h"

volatile unsigned int Task_Number;

std::string msg = "Hello World!";

void sciSendString(sciBASE_t *sci, std::string str) {
	for(int i=0; i < str.length(); ++i) {
		while( (scilinREG->FLR & 0x4) == 4);
		sciSendByte(scilinREG, str[i]);
	}
}

void sciSendBytes(sciBASE_t *sci, uint8_t * bytes, size_t length) {
	for(int i=0; i < length; ++i) {
		while( (scilinREG->FLR & 0x4) == 4);
		sciSendByte(scilinREG, bytes[i]);
	}
}

int main() {
	gioInit();
	sciInit();
	_enable_IRQ();
	sciSendString(scilinREG, msg);

	while(1) {
		switch(Task_Number) {
			case 1:
				Task_Number = 0;
				gioToggleBit(gioPORTA, 2);
				break;
		}
	}
}


