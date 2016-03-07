/******************************************************************************/
/*Files to Include                                                            */
/******************************************************************************/

#if defined(__XC)
#include <xc.h>         /* XC8 General Include File */
#endif

#if defined(__XC)
#include <stdint.h>         /* For uint8_t definition */
#include <stdbool.h>        /* For true/false definition */
#endif

#include <stdio.h>
#include "user.h"
#include "24lc64.h"

#ifndef I2C_SCL1

#define SCL P1_6
#define SDA P1_7

#endif

#define I2CWAIT 10
#define I2CADR 0xa0


bit error_24lc64;

void I2C_delay(void) {
    unsigned char x;
    for (x = 0; x < I2CWAIT; x++) {
    }
    CLRWDT();
}

void I2C_start(void) // assumes SDA and SCL are high at entry
{
    I2C_SDA1 = 0;
    I2C_delay();
    I2C_SCL1 = 0;
    I2C_delay();
}

void I2C_stop(void) // assumes SDA and SCL are low at entry
{
    I2C_SCL1 = 1;
    I2C_delay();
    I2C_SDA1 = 1;
    I2C_delay();
}

unsigned char I2C_read(int ack) // assume SDA and SCL are low at entry
{
    unsigned char x;
    unsigned char result = 0;
    I2C_SDA = 1; // SDA is input
    for (x = 0; x < 8; x++) {
        result = result << 1; // read 8 data bits
        I2C_delay();
        I2C_SCL1 = 1;
        I2C_delay();
        if (I2C_SDA1 == 1) result = result + 1;
        I2C_SCL1 = 0;
    }
    I2C_delay();
    I2C_SDA1 = !ack;
    I2C_delay();
    I2C_SCL1 = 1;
    I2C_delay();
    I2C_SCL1 = 0;
    I2C_delay();
    I2C_SDA1 = 0; // all our routines expect SDA and SCL
    //are low when chipcomm is in progress
    return result;
}

bit I2C_write(unsigned char adr) {
    unsigned char x;
    static bit result;
    for (x = 0; x < 8; x++) {
        I2C_SDA1 = ((adr & 0x80) == 0x80);
        adr = adr << 1;
        I2C_delay();
        I2C_SCL1 = 1;
        I2C_delay();
        I2C_SCL1 = 0;
        I2C_delay();
    }
    I2C_SDA1 = 1; // SDA goes into input-mode
    I2C_delay(); // wait to settle SDA level
    I2C_SCL1 = 1;
    I2C_delay();
    if (I2C_SDA1 == 0) result = 1;
    else result = 0;
    I2C_SCL1 = 0;
    I2C_delay();
    I2C_SDA1 = 0;
    if (result == 0) error_24lc64 = 1;
    return result;
}

int is_24lc64_present(void) {
    static bit result;
    I2C_start();
    result = I2C_write(0xa0);
    I2C_stop();
    return result;
}

void wait_24lc64_done(void) {
    static bit done = 0;
    unsigned char x = 0;
    error_24lc64 = 0;
    for (x = 0; x < 125; x++) {
        I2C_start();
        done = I2C_write(0xa0);
        I2C_stop();
        if (done == 1) break;
    }
    if (done == 0) error_24lc64 = 1;
}

unsigned char read_byte_24lc64(unsigned int adr) {
    unsigned char result;
    wait_24lc64_done(); // wait for the completion of a possible writeoperation
    I2C_start();
    I2C_write(0xa0); // select 24lc64 for writing;
    I2C_write((adr >> 8)); // write MSB of adrespointer
    I2C_write((adr & 0x00ff)); // write LSB of adrespoint
    I2C_stop();
    I2C_start();
    I2C_write(0xa1); // select 24lc64 for reading;
    result = I2C_read(0); // read without an ACK, to indicate that
    //this was the last readoperation
    I2C_stop();
    return result;
}

unsigned char read_next_byte_24lc64(void) {
    unsigned char result;
    I2C_start();
    I2C_write(0xa1);
    result = I2C_read(0);
    I2C_stop();
    return result;
}

void read_buffer_24lc64(unsigned int adr, unsigned char* dat,
        unsigned char count) {
    if (count == 1) {
        *dat = read_byte_24lc64(adr);
        return;
    }
    wait_24lc64_done(); // wait for the completion of a possible writeoperation
    I2C_start();
    I2C_write(0xa0);
    I2C_write((adr >> 8)); // write MSB of adrespointer
    I2C_write((adr & 0x00ff)); // write LSB of adrespoint
    I2C_stop();
    I2C_start();
    I2C_write(0xa1); // select 24lc64 for reading;
    *dat++ = I2C_read(1); // read with an ACK, to indicate that
    //this wasn't the last readoperation
    count--;
    while (count > 0) {
        count--;
        *dat++ = I2C_read(count != 0); // read next byte(s).
        //if it is the last one, don't ACK the comm.
    }
    I2C_stop();
    return; // close the I2C-bus
}

void write_byte_24lc64(unsigned int adr, unsigned char dat) {
    wait_24lc64_done(); // wait for the completion of a possible writeoperation
    I2C_start();
    I2C_write(0xa0); // select 24lc64;
    I2C_write((adr >> 8)); // write MSB of adrespointer
    I2C_write((adr & 0x00ff)); // write LSB of adrespoint
    I2C_write(dat);
    I2C_stop();
}
