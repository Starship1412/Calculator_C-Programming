#include "PmodKYPD.h"
#include "sleep.h"
#include "xil_cache.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "xtime_l.h"
#include <stdbool.h>

void DemoInitialize();
void DemoRun();
void CalculateResult(float *op1, float *op2, char *sign2, bool shouldUpdateOp2);
void PrintFloat(float value);
void Clear(char *sign1, char *sign2, bool *begin, bool *updateOp2, bool *isDecimal, bool *leadingZero, bool *needZero, bool *negative, float *op1, float *op2, float *decimal);
void initialize_keypad();
bool is_key_pressed(char target_key);
void DemoCleanup();
void EnableCaches();
void DisableCaches();
void DemoSleep(u32 millis);

PmodKYPD myDevice;

int main(void) {
	DemoInitialize();
	DemoRun();
	DemoCleanup();
	return 0;
}

// keytable is determined as follows (indices shown in Keypad position below)
// 12 13 14 15
// 8  9  10 11
// 4  5  6  7
// 0  1  2  3
#define DEFAULT_KEYTABLE "0FED789C456B123A"

void DemoInitialize() {
	EnableCaches();
	KYPD_begin(&myDevice, XPAR_PMODKYPD_0_AXI_LITE_GPIO_BASEADDR);
	KYPD_loadKeyTable(&myDevice, (u8*) DEFAULT_KEYTABLE);
}

void DemoRun() {
	u16 keystate;
	XStatus status, last_status = KYPD_NO_KEY;
	u8 key, last_key = 'x';
	// Initial value of last_key cannot be contained in loaded KEYTABLE string

	char sign_1 = '=';
	char sign_2 = '=';
	bool start = true;
	bool shouldUpdateOp2 = false;
	bool isDecimalMode = false;
	bool hasLeadingZero = false;
	bool needAddZero = false;
	bool negativeMode = false;
	bool exitCase = false;
	float first_op = 0;
	float second_op = 0;
	float decimalFactor = 0.1;
	XTime pressStartTime, currentTime;

	Xil_Out32(myDevice.GPIO_addr, 0xF);
	xil_printf("Pmod KYPD demo started. Press any key on the Keypad.\r\n");
	while (1) {
		// Capture state of each key
		keystate = KYPD_getKeyStates(&myDevice);

		// Determine which single key is pressed, if any
		status = KYPD_getKeyPressed(&myDevice, keystate, &key);

		// Print key detect if a new key is pressed or if status has changed
		if (status == KYPD_SINGLE_KEY && (status != last_status || key != last_key)) {
			switch (key) {
				case 'A':
					XTime_GetTime(&pressStartTime);
					while (is_key_pressed('A')) { // Enter positive number if pressed for 2 seconds
						XTime_GetTime(&currentTime);
						if (shouldUpdateOp2)
							break;
						if (1.0f * (currentTime - pressStartTime) / COUNTS_PER_SECOND >= 2) {
							if (!shouldUpdateOp2) {
								negativeMode = false;
								xil_printf("+");
								exitCase = true;
							}
							break;
						}
					}
					if (exitCase) {
						exitCase = false;
						break;
					}
					if (!start) { // Addition +
						if (isDecimalMode && needAddZero)
							xil_printf("0");
						sign_1 = '+';
						if (shouldUpdateOp2 && first_op == 0 && sign_2 == '/') {
							xil_printf("\r\nDividend is 0! Not allowed! Clear! Start over.\r\n");
							Clear(&sign_1, &sign_2, &start, &shouldUpdateOp2, &isDecimalMode, &hasLeadingZero, &needAddZero, &negativeMode, &first_op, &second_op, &decimalFactor);
							break;
						}
						CalculateResult(&first_op, &second_op, &sign_2, shouldUpdateOp2);
						if (shouldUpdateOp2)
							xil_printf("\r\n");
						if ((sign_2 == '+' || sign_2 == '-' || sign_2 == '*' || sign_2 == '/') && shouldUpdateOp2)
							PrintFloat(second_op);
						xil_printf("+\r\n");
						sign_2 = sign_1;
						shouldUpdateOp2 = false;
						isDecimalMode = false;
						hasLeadingZero = false;
						negativeMode = false;
						decimalFactor = 0.1;
					}
					break;
				case 'B':
					XTime_GetTime(&pressStartTime);
					while (is_key_pressed('B')) { // Enter negative number if pressed for 2 seconds
						XTime_GetTime(&currentTime);
						if (shouldUpdateOp2)
							break;
						if (1.0f * (currentTime - pressStartTime) / COUNTS_PER_SECOND >= 2) {
							if (!shouldUpdateOp2) {
								negativeMode = true;
								xil_printf("-");
								exitCase = true;
							}
							break;
						}
					}
					if (exitCase) {
						exitCase = false;
						break;
					}
					if (!start) { // Subtraction -
						if (isDecimalMode && needAddZero)
							xil_printf("0");
						sign_1 = '-';
						if (shouldUpdateOp2 && first_op == 0 && sign_2 == '/') {
							xil_printf("\r\nDividend is 0! Not allowed! Clear! Start over.\r\n");
							Clear(&sign_1, &sign_2, &start, &shouldUpdateOp2, &isDecimalMode, &hasLeadingZero, &needAddZero, &negativeMode, &first_op, &second_op, &decimalFactor);
							break;
						}
						CalculateResult(&first_op, &second_op, &sign_2, shouldUpdateOp2);
						if (shouldUpdateOp2)
							xil_printf("\r\n");
						if ((sign_2 == '+' || sign_2 == '-' || sign_2 == '*' || sign_2 == '/') && shouldUpdateOp2)
							PrintFloat(second_op);
						xil_printf("-\r\n");
						sign_2 = sign_1;
						shouldUpdateOp2 = false;
						isDecimalMode = false;
						hasLeadingZero = false;
						negativeMode = false;
						decimalFactor = 0.1;
					}
					break;
				case 'C': // Multiplication *
					if (!start) {
						if (isDecimalMode && needAddZero)
							xil_printf("0");
						sign_1 = '*';
						if (shouldUpdateOp2 && first_op == 0 && sign_2 == '/') {
							xil_printf("\r\nDividend is 0! Not allowed! Clear! Start over.\r\n");
							Clear(&sign_1, &sign_2, &start, &shouldUpdateOp2, &isDecimalMode, &hasLeadingZero, &needAddZero, &negativeMode, &first_op, &second_op, &decimalFactor);
							break;
						}
						CalculateResult(&first_op, &second_op, &sign_2, shouldUpdateOp2);
						if (shouldUpdateOp2)
							xil_printf("\r\n");
						if ((sign_2 == '+' || sign_2 == '-' || sign_2 == '*' || sign_2 == '/') && shouldUpdateOp2)
							PrintFloat(second_op);
						xil_printf("*\r\n");
						sign_2 = sign_1;
						shouldUpdateOp2 = false;
						isDecimalMode = false;
						hasLeadingZero = false;
						negativeMode = false;
						decimalFactor = 0.1;
					}
					break;
				case 'D': // Division /
					if (!start) {
						if (isDecimalMode && needAddZero)
							xil_printf("0");
						sign_1 = '/';
						if (shouldUpdateOp2 && first_op == 0 && sign_2 == '/') {
							xil_printf("\r\nDividend is 0! Not allowed! Clear! Start over.\r\n");
							Clear(&sign_1, &sign_2, &start, &shouldUpdateOp2, &isDecimalMode, &hasLeadingZero, &needAddZero, &negativeMode, &first_op, &second_op, &decimalFactor);
							break;
						}
						CalculateResult(&first_op, &second_op, &sign_2, shouldUpdateOp2);
						if (shouldUpdateOp2)
							xil_printf("\r\n");
						if ((sign_2 == '+' || sign_2 == '-' || sign_2 == '*' || sign_2 == '/') && shouldUpdateOp2)
							PrintFloat(second_op);
						xil_printf("/\r\n");
						sign_2 = sign_1;
						shouldUpdateOp2 = false;
						isDecimalMode = false;
						hasLeadingZero = false;
						negativeMode = false;
						decimalFactor = 0.1;
					}
					break;
				case 'E':
					XTime_GetTime(&pressStartTime);
					while (is_key_pressed('E')) { // Clear if pressed for 3 seconds
						XTime_GetTime(&currentTime);
						if (1.0f * (currentTime - pressStartTime) / COUNTS_PER_SECOND >= 3) {
							if (shouldUpdateOp2)
								xil_printf("\r\n");
							xil_printf("Clear!\r\n");
							Clear(&sign_1, &sign_2, &start, &shouldUpdateOp2, &isDecimalMode, &hasLeadingZero, &needAddZero, &negativeMode, &first_op, &second_op, &decimalFactor);
							exitCase = true;
							break;
						}
					}
					if (exitCase) {
						exitCase = false;
						break;
					}
					if (!start) { // Equal =
						if (isDecimalMode && needAddZero)
							xil_printf("0");
						sign_1 = '=';
						if (shouldUpdateOp2 && first_op == 0 && sign_2 == '/') {
							xil_printf("\r\nDividend is 0! Not allowed! Clear! Start over.\r\n");
							Clear(&sign_1, &sign_2, &start, &shouldUpdateOp2, &isDecimalMode, &hasLeadingZero, &needAddZero, &negativeMode, &first_op, &second_op, &decimalFactor);
							break;
						}
						CalculateResult(&first_op, &second_op, &sign_2, shouldUpdateOp2);
						if (shouldUpdateOp2)
							xil_printf("\r\n");
						if (sign_2 != '=') {
							xil_printf("=\r\n");
							PrintFloat(second_op); // fflush(stdout);
						}
						sign_2 = sign_1;
						shouldUpdateOp2 = false;
						isDecimalMode = false;
						hasLeadingZero = false;
						needAddZero = false;
						negativeMode = false;
						decimalFactor = 0.1;
					}
					break;
			    case 'F': // Decimal Point .
			        if (!isDecimalMode) {
			            if (first_op == 0 && !hasLeadingZero) {
			            	xil_printf("0");
			            	hasLeadingZero = true;
			            }
			            isDecimalMode = true;
			            xil_printf(".");
			        }
					start = false;
					shouldUpdateOp2 = true;
					needAddZero = true;
			        break;
				default:
			        if (isDecimalMode) {
			        	if (negativeMode)
			        		first_op = first_op - (key - '0') * decimalFactor;
			        	else
			        		first_op = first_op + (key - '0') * decimalFactor;
			            decimalFactor = decimalFactor * 0.1;
			            if (!(key == 'F'))
			            	needAddZero = false;
			        } else {
			        	if (negativeMode)
			        		first_op = first_op * 10.0 - (key - '0');
			        	else
			        		first_op = first_op * 10.0 + (key - '0');
						if (first_op == 0 && key == '0' && !hasLeadingZero) {
							xil_printf("0");
							hasLeadingZero = true;
						}
			        }
			        if (!(first_op == 0 && key == '0' && !isDecimalMode))
			        	xil_printf("%c", (char)key);
					start = false;
					shouldUpdateOp2 = true;
					break;
			}
			last_key = key;
		} else if (status == KYPD_MULTI_KEY && status != last_status)
			xil_printf("Error: Multiple keys pressed\r\n");
		last_status = status;
		usleep(1000);
	}
}

void CalculateResult(float *op1, float *op2, char *sign2, bool updateOp2) {
	if (updateOp2)
		switch (*sign2) {
			case '+':
				*op2 = *op2 + *op1;
				break;
			case '-':
				*op2 = *op2 - *op1;
				break;
			case '*':
				*op2 = *op2 * *op1;
				break;
			case '/':
				*op2 = *op2 / *op1;
				break;
			default:
				*op2 = *op1;
				break;
		}
	*op1 = 0;
}

void PrintFloat(float value) {
    int int_part = (int)value;
    float frac = value - int_part;
    if (value < 0) {
		xil_printf("-");
		int_part = -int_part;
		frac = -frac;
	}
    int frac_int = (int)(frac * 10000);
    int frac_last_digit = frac_int % 10;
    if (frac_last_digit >= 5) {
        frac_int = frac_int / 10 + 1;
    } else {
        frac_int = frac_int / 10;
    }
    if (frac_int == 0) {
        xil_printf("%d\r\n", int_part);
    } else {
        if (frac_int % 10 == 0) {
            frac_int /= 10;
            if (frac_int % 10 == 0) {
                frac_int /= 10;
                xil_printf("%d.%d\r\n", int_part, frac_int);
            } else {
                xil_printf("%d.%02d\r\n", int_part, frac_int);
            }
        } else {
            xil_printf("%d.%03d\r\n", int_part, frac_int);
        }
    }
}

void Clear(char *sign1, char *sign2, bool *begin, bool *updateOp2, bool *isDecimal, bool *leadingZero, bool *needZero, bool *negative, float *op1, float *op2, float *decimal) {
	*sign1 = '=';
	*sign2 = '=';
	*begin = true;
	*updateOp2 = false;
	*isDecimal = false;
	*leadingZero = false;
	*needZero = false;
	*negative = false;
	*op1 = 0;
	*op2 = 0;
	*decimal = 0.1;
}

void initialize_keypad() {
    KYPD_begin(&myDevice, XPAR_PMODKYPD_0_AXI_LITE_GPIO_BASEADDR);
    KYPD_loadKeyTable(&myDevice, (u8*)DEFAULT_KEYTABLE);
}

bool is_key_pressed(char target_key) {
    u16 keystate = KYPD_getKeyStates(&myDevice);
    u8 detected_key;
    XStatus status = KYPD_getKeyPressed(&myDevice, keystate, &detected_key);
    return (status == KYPD_SINGLE_KEY && detected_key == target_key);
}

void DemoCleanup() {
	DisableCaches();
}

void EnableCaches() {
#ifdef __MICROBLAZE__
#ifdef XPAR_MICROBLAZE_USE_ICACHE
	Xil_ICacheEnable();
#endif
#ifdef XPAR_MICROBLAZE_USE_DCACHE
	Xil_DCacheEnable();
#endif
#endif
}

void DisableCaches() {
#ifdef __MICROBLAZE__
#ifdef XPAR_MICROBLAZE_USE_DCACHE
	Xil_DCacheDisable();
#endif
#ifdef XPAR_MICROBLAZE_USE_ICACHE
	Xil_ICacheDisable();
#endif
#endif
}
