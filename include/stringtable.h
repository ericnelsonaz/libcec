#ifndef __STRINGTABLE_H__
#define __STRINGTABLE_H__

/*
 * simple map of unsigned -> char const *
 *
 * Implementations should be sorted based on the integer
 * value and the sort_string_table routine is available
 * to make this easy.
 */

struct string_table_t {
    unsigned ival;
    char const *sval;
};

/*
 * utility routine to qsort() the array
 */
void sort_string_table(struct string_table_t *tab,
                       unsigned count);

/*
 * returns NULL if not found
 */
char const *find_string(unsigned key,
                        struct string_table_t *tab,
                        unsigned count);

#endif
