#ifndef __UINPUT_KEYS_H__
#define __UINPUT_KEYS_H__

#include "stringtable.h"

extern struct string_table_t *uinput_keys;
extern unsigned num_uinput_keys;

/*
 * returns NULL if not found
 */
extern char const *find_uinput_key(unsigned key);

#endif
