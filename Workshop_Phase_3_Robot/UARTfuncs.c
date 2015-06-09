#include "msp430g2553.h"
#include <stdarg.h>
#include <stdio.h>

#define UART_PRINTF_SIZE 25
#define MAX_NUM_FLOATS 10

char txbuff[MAX_NUM_FLOATS*5+1];
char rxbuff[MAX_NUM_FLOATS*5 + 1];
char printbuff[UART_PRINTF_SIZE];
char donesending = 1;

signed char txcount = 0;   
signed char currentindex = 0;
signed char senddone = 1;
char printf_flag = 0, UART_flag = 0;

int sendchar(char c) {

  if (senddone == 1) {
    senddone = 0;
    UCA0TXBUF = c;

    return(0);
  } else {
    return(-1); // error
  }
}
// This function assumes txbuff has already been filled with the characters to send.
// It initializes txcount to the number of chars to send and then sets senddone = 0 so that txbuff is sent out UCATX
int sendchars(int size) {

  if (senddone == 1) {  // Only setup txcount if previous transmission complete
    if (size < UART_PRINTF_SIZE) {
      txcount = size;
    } else {
      txcount = UART_PRINTF_SIZE;
    }

    currentindex = 1;
    senddone = 0;  // signal that a new transmission should occur.
    UCA0TXBUF = printbuff[0];

    return(0);
  } else {
    return(-1); // error
  }

}

int UART_printf(const char *format, ...)               
{
  // the "va" and "v" functions handle the variable argument ... in the function parameters
  va_list ap;
  int error;

	printf_flag = 1;

  va_start(ap, format);      /* Variable argument begin */
  error = sendchars(vsprintf(printbuff, format, ap));   // fill txbuff with the format string and the pass the size of txbuff to the sendchars function
  va_end(ap);                /* Variable argument end */
  return error;
}

// Converts a 32-bit float into 5 characters for UART transmission
// Inputs are floating-point number to convert and an array pointer to store
// bits in.  Bits are stored from least-significant 7 (in [0]-element) to
// most-significant 4 (in [4]-element)
void float2chars(float input, char* parts) {

	union {
		unsigned long bits;
		float number;
	} floatbits;

	floatbits.number = input;

	parts[0] = floatbits.bits & ~0x80; // clear top bit
	parts[1] = (floatbits.bits >> 7) & ~0x80;
	parts[2] = (floatbits.bits >> 14) & ~0x80;
	parts[3] = (floatbits.bits >> 21) & ~0x80;
	parts[4] = (floatbits.bits >> 28);

}


// Converts a 5-element array of chars received via UART to a 32-bit float
// Input pointer to array of chars
// Bits are decoded from least significant 7 (in [0]-element) to most 
// significant 4 (in [4]-element)
float chars2float(char* parts) {

		union {
		unsigned long bits;
		float number;
	} floatbits;

	floatbits.bits = parts[0] + ((long)parts[1]<<7) + ((long)parts[2]<<14) + 
															((long)parts[3]<<21) + ((long)parts[4]<<28);

	return floatbits.number;
}




void my_scanf(char* rawmessage,...) {
	char endindex = 0, i = 0;
	va_list a_list;

	while(rawmessage[endindex] != 255) {
		endindex++;

		if(endindex > MAX_NUM_FLOATS*5) return;
	}

	va_start(a_list,rawmessage);

	for(i=0;i<endindex; i=i+5) {
		*va_arg(a_list,float *) = chars2float(&rawmessage[i]);
	}

}


void UART_send(int numargs,...) {
	va_list a_list;
	char i = 0;

	if(numargs > MAX_NUM_FLOATS || !donesending) return;	// error

	va_start(a_list,numargs);

	for(i = 0; i<numargs; i++) {
		float2chars((float)va_arg(a_list,double),&txbuff[5*i]);		
	}

	va_end(a_list);

	txbuff[5*numargs] = 255;

	UART_flag = 1;
	donesending = 0;
	UCA0TXBUF = 253;

}


// USCI_A Initialization - UART mode
// Assumes SMCLK is running at 16MHz
// Inputs: baud rate and os = 0 or 1 indicating whether to use oversampling mode
void Init_UART(unsigned long baudrate, char os) {
	float n = 0;
	char BRFx = 0, BRSx = 0;
	int BRx = 0;

	if(os > 1) os = 1;	// error check since os is a logical

	n = 16.0e6/baudrate;

	UCA0CTL1 = UCSSEL_2 + UCSWRST;           // source SMCLK, hold module in reset

	if(os && (n >= 16)) {										// Oversampling mode
		BRx = (int)(n/16);										// Baud rate selection
		BRFx = (int)(((n/16)-BRx)*16 + 0.5);	// Modulator selection
		UCA0MCTL = UCOS16 + (BRFx<<4);
	}
	else {																	// Normal mode
		BRx = (int)n;													// Baud rate selection
		BRSx = (int)((n-BRx)*8 + 0.5);				// Modulator selection
		UCA0MCTL = BRSx<<1;
	}

	UCA0BR0 = BRx % 256;
	UCA0BR1 = BRx / 256;

	//msp430G2553
	P1SEL |= 0x6;
	P1SEL2 |= 0x6;
	//P3SEL |= 0x30;                            // P3.4,5 = USCI_A0 TXD/RXD
	UCA0CTL1 &= ~UCSWRST;                     // Release USCI module for operation
	IFG2 &= ~(UCA0TXIFG + UCA0RXIFG);					// Clear pending interrupt flags
  IE2 |= UCA0TXIE;                        	// Enable USCI_A0 TX interrupt
  IE2 |= UCA0RXIE;                        	// Enable USCI_A0 RX interrupt

}


