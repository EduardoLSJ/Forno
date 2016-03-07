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
#include <string.h>
#include "user.h"
#include "keys.h"

// Switches input
#define SW_MASK      0x38

unsigned char keypress, prev_keypress, key_state;

void init_keys(void) {
    keypress = KEYRELEASED;
    prev_keypress = KEYRELEASED;
    key_state = KEYRELEASED;
}

unsigned char keys_get(void) {
    static volatile unsigned debounce = DEBOUNCEVAL;
    keypress = ~KEY_bits & SW_MASK;
    CLRWDT();
    if (keypress == prev_keypress) { //key repeated press
        --debounce;
        if (debounce)
            return KEYRELEASED; //retorna tecla solta, antes de debounce
        switch (key_state) {
            case KEYRELEASED: //retorna com valor de tecla solta
                key_state = KEYPRESS;
                debounce = KEYFIRSTREPEATVAL;
                break;
            case KEYPRESS: //debounce corrigido
                key_state = KEYPRESSLONG;
                debounce = KEYREPEATVAL;
                break;
            case KEYPRESSLONG:
                debounce = KEYREPEATVAL;
                break;
            default:
                break;
        }
        return keypress;
    }
    debounce = DEBOUNCEVAL;
    key_state = KEYRELEASED;
    prev_keypress = keypress;
    return KEYRELEASED; //nova tecla pressionada?
}
