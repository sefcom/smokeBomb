#ifndef ID3_H_INCLUDED
#define ID3_H_INCLUDED


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
	long				winvalue;
	long				tot_nodes;
	long				nodes[6];
#if defined(__arm__) && !defined(__aarch64__) 
	char				dummy[32];					
#endif
} pre_node_t __attribute__((aligned (64)));

struct pre_dsinfo_t
{
	char name[64];
	long value;
};

/* test to classify one record */
long test_prestored_tree ( void );
long test_prestored_tree_with_data(char **data);

#endif // ID3_H_INCLUDED
