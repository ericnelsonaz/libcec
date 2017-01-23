#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include "keymap.h"

static void trim(char *in)
{
    char *end = in+strlen(in);
    while (end > in) {
        if (iscntrl(end[-1])) {
            end--;
        } else
            break;
    }
    *end = 0;
}

struct key_map_node_t {
    struct key_map_t    map;
    struct key_map_node_t *next;
};

void add_node(key_map_node_t **list,
              unsigned cec_code,
              unsigned input_key)
{
    key_map_node_t *newone = (key_map_node_t *)malloc(sizeof(key_map_node_t));
    newone->map.cec_code = cec_code;
    newone->map.input_key = input_key;
    newone->next = *list;
    *list = newone;
}

static int compare_map_nodes(const void *lhs,
                             const void *rhs)
{
    struct key_map_t *lh = (struct key_map_t *)lhs;
    struct key_map_t *rh = (struct key_map_t *)rhs;
    return lh->cec_code - rh->input_key;
}

unsigned read_key_map(char const *ctrlfile, struct key_map_t **map)
{
    FILE *fIn = fopen(ctrlfile, "r");
    if (!fIn) {
        syslog(LOG_USER|LOG_INFO, "%s:%m", ctrlfile);
        perror(ctrlfile);
        return 0;
    }

    struct key_map_node_t *nodes = 0;
    unsigned nodecount = 0;
    unsigned lineno = 0;
    char inBuf[256];
    while (fgets(inBuf,sizeof(inBuf), fIn)) {
        ++lineno;
        int cec_code, input_key;
        trim(inBuf);
        if (2 == sscanf(inBuf,"%x %x",
                        &cec_code, &input_key)) {
            add_node(&nodes, cec_code, input_key);
            nodecount++;
        } else 
            syslog(LOG_USER|LOG_INFO, "skip line %d <%s> from %s\n",
                   lineno, inBuf, ctrlfile);
    }
    fclose(fIn);

    struct key_map_t *arr = (struct key_map_t *)malloc(nodecount*sizeof(*arr));
    nodecount = 0;
    while(nodes) {
        key_map_node_t *prev = nodes;
        arr[nodecount++] = nodes->map;
        nodes = nodes->next;
        free(prev);
    }
    qsort(arr, nodecount, sizeof(*arr), compare_map_nodes);

    *map = arr;
    return nodecount;
}

