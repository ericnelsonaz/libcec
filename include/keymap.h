#ifndef __KEYMAP_H__
#define __KEYMAP_H__
/*
 * Utility to read a configuration file to map CEC keys to
 * Linux cec_code keys.
 *
 * The text file should contain one line with two hex numbers for 
 * each translation.
 *
 */

struct key_map_t {
    unsigned cec_code;
    unsigned input_key;
};

/*
 * returns number of entries parsed
 */
unsigned read_key_map(char const *ctrlfile, struct key_map_t **map);

#endif
