#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include "cec-keys.h"
#include "input-keys.h"
#include "uinput-keys.h"
#include "keymap.h"

int main (int argc, char *argv[])
{
    if (2 > argc) {
        fprintf(stderr, "Usage: %s /path/to/config.file\n",
                argv[0]);
        return -1;
    }

    struct key_map_t *keymap;
    unsigned num_keys = read_key_map(argv[1], &keymap);
    printf("read %u keys into keymap\n", num_keys);

    for (unsigned i=0; i < num_keys; i++) {
        unsigned cec = keymap[i].cec_code;
        unsigned input = keymap[i].input_key;
        printf("0x%02x(%s) mapped to 0x%02x(%s)\n", 
               cec, find_cec_key(cec),
               input, find_input_key(input));
        printf("\tjavascript mapping %s\n", find_uinput_key(input));
    }
    return 0;
}
