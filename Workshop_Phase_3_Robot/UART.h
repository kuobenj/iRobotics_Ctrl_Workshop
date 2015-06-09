#ifndef UART_PRINTF_H_
#define UART_PRINTF_H_

#define UART_PRINTF_SIZE 25
#define MAX_NUM_FLOATS 10

int UART_printf(const char *format, ...);
void float2chars(float input, char* parts);
float chars2float(char* parts);
void my_scanf(char* rawmessage,...);
void UART_send(int numargs,...);
void Init_UART(unsigned long baudrate, char os);
int sendchar(char c);

extern char printbuff[UART_PRINTF_SIZE];
extern char txbuff[MAX_NUM_FLOATS*5+1];
extern char rxbuff[MAX_NUM_FLOATS*5+1];
extern signed char txcount, currentindex, senddone;
extern char printf_flag, donesending, UART_flag;

char msgindex = 0, txindex = 0, started = 0, newmsg = 0;


#endif

