#include "stringtable.h"
#include <stdlib.h>

static int compare_string_table(const void *lhs, const void *rhs)
{
    return ((struct string_table_t const *)lhs)->ival
            - ((struct string_table_t const *)rhs)->ival;
}

/*
 * utility routine to qsort() the array
 */
void sort_string_table(struct string_table_t *tab,
                       unsigned count)
{
    qsort(tab, count, sizeof(*tab), compare_string_table);
}

static int compare_str_to_table(const void *key, const void *tab)
{
    return *((unsigned *)key)
            - ((struct string_table_t const *)tab)->ival;
}

/*
 * returns NULL if not found
 */
char const *find_string(unsigned key,
                        struct string_table_t *tab,
                        unsigned count)
{
    void *val = bsearch(&key, tab, count, sizeof(*tab),
                        compare_str_to_table);
    return val ? ((struct string_table_t *)val)->sval : 0;
}

