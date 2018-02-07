#ifndef ID3_H_INCLUDED
#define ID3_H_INCLUDED

#include <stdint.h>

/*
    Struttura dati info dataset
*/
struct dsinfo_t
{
    char            	*name;
    long            	value;
	long				column;
    struct dsinfo_t   	*next;
    struct dsinfo_t   	*prev;
};

/*
	Struttura dati foglia
*/
typedef struct node_tag
{
	long				winvalue;
	long				tot_attrib;
	long				*avail_attrib;
	long				tot_samples;
	long				*samples;
	long				tot_nodes;
	struct node_tag	*nodes;
} node_t;

typedef struct pre_node_tag
{
	int				winvalue;
	int				tot_nodes;
	int				nodes[6]; 
	char			dummy[32];
} pre_node_t __attribute__((aligned (64)));

struct pre_dsinfo_t
{
	char name[64];
	int value;
};

/* test to classify one record */
int test_prestored_tree ( void );
int test_prestored_tree_with_data(char **data);

void sb_get_time(uint64_t *time, unsigned int *count);

#endif // ID3_H_INCLUDED
