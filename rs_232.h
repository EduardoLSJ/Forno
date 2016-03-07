#ifndef _RS_232_H_
#define _RS_232_H_

#define BAUDRATE 9600
#define USART_BITS 8
#define USART_PARITY N
#define USART_STOP  1

void init_rs232(void);
void printstr_rs232(char* str);
void printchar_rs232(char ch);

#endif
