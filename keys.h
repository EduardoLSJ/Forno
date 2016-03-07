#ifndef _KEYS_H_
#define _KEYS_H_

#define KEY_UP    0x28
#define KEY_DOWN  0x18
#define KEY_LEFT  0x10
#define KEY_RIGHT 0x20
#define KEY_ENTER 0x08
#define KEY_EXIT  0x30

// keyboard const
#define DEBOUNCEVAL  30
#define KEYFIRSTREPEATVAL 150
#define KEYREPEATVAL 50

#define KEYRELEASED 0
#define KEYPRESS 1
#define KEYPRESSLONG 2

void init_keys(void);
unsigned char keys_get(void);

#endif
