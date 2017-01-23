#ifndef __CEC_KEYS_H__
#define __CEC_KEYS_H__

#include "stringtable.h"

extern struct string_table_t *cec_keys;
extern unsigned num_cec_keys;

/*
 * returns NULL 
 */
extern char const *find_cec_key(unsigned key);

#endif
