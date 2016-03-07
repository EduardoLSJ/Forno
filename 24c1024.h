#ifndef _24C1024_H_
#define _24C1024_H_

int is_24c1024_present(void);
//extern bit error_24lc64;

void I2C_init(void);
unsigned char read_byte_24c1024(unsigned int adr);
void write_byte_24c1024(unsigned int adr, unsigned char dat);
unsigned char read_next_byte_24c1024(void);
void read_buffer_24c1024(unsigned int adr, unsigned char* dat,
        unsigned char count);
#endif
