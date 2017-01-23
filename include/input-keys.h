#ifndef __INPUT_KEYS_H__
#define __INPUT_KEYS_H__

#include "stringtable.h"

extern struct string_table_t *input_keys;
extern unsigned num_input_keys;

/*
 * returns NULL if not found
 */
extern char const *find_input_key(unsigned key);

#endif
