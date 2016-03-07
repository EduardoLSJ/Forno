#ifndef _24LC64_H_
#define _24LC64_H_

int is_24lc64_present(void);
//extern bit error_24lc64;

unsigned char read_byte_24lc64(unsigned int adr);
void write_byte_24lc64(unsigned int adr, unsigned char dat);
unsigned char read_next_byte_24lc64(void);
void read_buffer_24lc64(unsigned int adr, unsigned char* dat, unsigned char count);
#endif
