

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#include "id3.h"

//#define DO_DEBUG

#ifdef DO_DEBUG
	#define	DEBUG	printf
#else
	#define	DEBUG(...) do{}while(0);
#endif

#include <time.h>
#include <stdint.h>
/* get nanosecond time. 10^-9 */
uint64_t get_monotonic_time(void)
{
    struct timespec t1;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    return t1.tv_sec * 1000*1000*1000ULL + t1.tv_nsec;
}

uint64_t sb_time_sum = 0;
unsigned int sb_test_count = 0;

/* precreated tree by create_prestored_tree() function */
const pre_node_t dec_tree[32] __attribute__((aligned (4096))) = {
	[31] = {
	.winvalue = 31,
	.tot_nodes = 3,
	.nodes = {0,6,8,},
	},
	[0] = {
	.winvalue = 0,
	.tot_nodes = 2,
	.nodes = {2,11,},
	},
	[2] = {
	.winvalue = 2,
	.tot_nodes = 1,
	.nodes = {4,},
	},
	[4] = {
	.winvalue = 4,
	.tot_nodes = 0,
	.nodes = {},
	},
	[11] = {
	.winvalue = 11,
	.tot_nodes = 1,
	.nodes = {7,},
	},
	[7] = {
	.winvalue = 7,
	.tot_nodes = 0,
	.nodes = {},
	},
	[6] = {
	.winvalue = 6,
	.tot_nodes = 1,
	.nodes = {7,},
	},
	[7] = {
	.winvalue = 7,
	.tot_nodes = 0,
	.nodes = {},
	},
	[8] = {
	.winvalue = 8,
	.tot_nodes = 2,
	.nodes = {3,5,},
	},
	[3] = {
	.winvalue = 3,
	.tot_nodes = 1,
	.nodes = {7,},
	},
	[7] = {
	.winvalue = 7,
	.tot_nodes = 0,
	.nodes = {},
	},
	[5] = {
	.winvalue = 5,
	.tot_nodes = 1,
	.nodes = {4,},
	},
	[4] = {
	.winvalue = 4,
	.tot_nodes = 0,
	.nodes = {},
	},
};
const struct pre_dsinfo_t dec_tree_infolist[] = {
    {
        .name = "SUNNY",
        .value = 0,
    },
    {
        .name = "HOT",
        .value = 1,
    },
    {
        .name = "HIGH",
        .value = 2,
    },
    {
        .name = "WEAK",
        .value = 3,
    },
    {
        .name = "NO",
        .value = 4,
    },
    {
        .name = "STRONG",
        .value = 5,
    },
    {
        .name = "OVERCAST",
        .value = 6,
    },
    {
        .name = "YES",
        .value = 7,
    },
    {
        .name = "RAIN",
        .value = 8,
    },
    {
        .name = "MILD",
        .value = 9,
    },
    {
        .name = "COOL",
        .value = 10,
    },
    {
        .name = "NORMAL",
        .value = 11,
    },
};

void sb_get_time(uint64_t *time, unsigned int *count)
{
	*time = sb_time_sum;
	*count = sb_test_count;
}

/*
	Prima scansione dell'albero di decisione per raccoglioere
	info riguardo alla profondita' massima dei rami e al numero
	massimo di regole create
*/
void scantree( node_t *node, long *max_depth, long *max_rules  )
{
	static int depth = 0;
	int j, i;

	if( node != NULL )
	{
		depth += 1;

		// memorizza la profondita' max dei rami
		if( depth > *max_depth ) *max_depth = depth;

		// memorizza il numero max di regole trovate
		if( node->tot_nodes == 0 ) *max_rules += 1;

		j = 0;
		while( j < node->tot_nodes )
		{
			scantree( node->nodes+j, max_depth, max_rules );

			depth -= 1;
			++j;
		}
	}
}

char *testset[] = 
{
	"RAIN",		"MILD",   	"HIGH",   	"WEAK",		"YES",
};

long *convert_str_data_to_int( char **data, int cols, int rows )
{
	int *dataset = NULL;
	unsigned int dataset_sz = 0;
	char label_found	= 0;
	int assign_id		= 0;
	int i, j, col, arr_size;

	dataset_sz = sizeof( unsigned int ) * cols * rows;

	// Allocazione memoria per la conversione stringa->value
	if( ( dataset = malloc( dataset_sz ) ) == NULL )
	{
		return NULL;
	}
	// azzero la tabella completamente
	memset( dataset, 0, dataset_sz );

	i = 0, col = 0;
	arr_size = sizeof(dec_tree_infolist) / sizeof(struct pre_dsinfo_t);
	while( i < (cols*rows) )
	{
		label_found	= 0;
		for(j=0; j<arr_size; j++)
		{
			if( !strcmp( dec_tree_infolist[j].name, data[ i ] ) )
			{
				label_found = 1;
				assign_id	= dec_tree_infolist[j].value;
				break;
			}
		}

		if( label_found == 0 )
		{
			/* error case */
			return NULL;
		}

		// aggiorno il corrispondente long nella tabella di conversione string->long
		dataset[ i ] = assign_id;

		// la variabile col tiene conto della colonna corrente all'interno del dataset
		// in caso di accodamento tiene traccia dell'attributo/classe a cui appartiene l'elemento
		if( ++col >= cols ) col = 0;
		// incremento indice di scorrimento elementi nel dataset
		i += 1;
	}

	/* print dataset */
	/*
	printf("\ntestset : \n");
	i = 0, col = 0;
	while( i < (cols*rows) )
	{
		printf("%d, ", dataset[i]);

		if(++col >= cols) {
			col = 0;
			printf("\n");
		}
		i++;
	}*/

	return dataset;
}

void print_prestored_node( node_t *node )
{
	int i;
	long winvalue;

	winvalue = node->winvalue;
	if(winvalue == -1)
		winvalue = 31;

	printf("\t[%ld] = {\n", winvalue);
	printf("\t\t.winvalue = %ld,\n", winvalue);
	printf("\t\t.tot_nodes = %ld,\n", node->tot_nodes);
	printf("\t\t.nodes = {");
	for(i=0; i<node->tot_nodes; i++)
	{
		printf("%d,", (node->nodes + i)->winvalue);
	}
	printf("},\n");
	printf("\t},\n");
}

void print_prestored_tree( node_t *node )
{
	int i;

	if(node != NULL)
	{
		if(node->tot_nodes == 0)
		{
			print_prestored_node(node);
			return;
		}
		
		print_prestored_node(node);
		for(i=0; i<node->tot_nodes; i++)
			print_prestored_tree(node->nodes + i);
	}
}

void print_prestored_infolist (struct dsinfo_t *infolist)
{
	struct dsinfo_t	*insptr = NULL;

	insptr = infolist;
	do {
		printf("\t{\n");
		printf("\t\t.name = \"%s\",\n", insptr->name);
		printf("\t\t.value = %ld,\n", insptr->value);
		printf("\t},\n");
		insptr = insptr->next;
	} while( insptr != NULL );
}

void create_prestored_tree( node_t *node, struct dsinfo_t *infolist )
{
	printf("const pre_node_t dec_tree[32] __attribute__((aligned (64))) = {\n");
	print_prestored_tree(node);
	printf("};\n");

	printf("\n");
	printf("const struct pre_dsinfo_t dec_tree_infolist[] = {\n");
	print_prestored_infolist(infolist);
	printf("};\n");
}

#ifdef SMOKE_BOMB_ENABLE
#include <sb_api.h>
#include <stdint.h>

#ifdef __aarch64__
#define get_current_pc(va) do { \
	asm volatile("adr %0, .\n" : "=r" (va)); \
}while(0)
#else
#define get_current_pc(va) do { \
	asm volatile ("mov %0, pc\n" : "=r" (va)); \
	va -= 8; \
}while(0)
#endif

unsigned long sva = 0, eva = 0;
int sb_init_flag = 0;
#endif

static int _traverse_prestored_tree(pre_node_t *root, int idx, int *dataset)
{
	int i, cidx;
	int flag = 0;
	int winvalue = -1, tidx = idx;
	uint64_t bc, ac;
	
#ifdef SMOKE_BOMB_ENABLE
	int sb_ret = 0;
    int sched_policy = -1, sched_prio = -1;
    uint64_t btime, atime;

    if (sva && eva && sb_init_flag == 0) {
        sb_ret = smoke_bomb_init(sva, eva, dec_tree, sizeof(pre_node_t) * 12, &sched_policy, &sched_prio);
        if (sb_ret) {
            printf("smoke_bomb_init error : %d\n", sb_ret);
            //exit(-1);
        } else {
            sb_init_flag = 1;
        }
    }

    asm volatile("nop"); asm volatile("nop");
    if (sva == 0) {
	    get_current_pc(sva);
	}
#endif
	
	bc = get_monotonic_time();
	do {
		flag = 0;
		
		if (root[tidx].tot_nodes == 0) {
			winvalue = root[tidx].winvalue;
			break;
		}

		for (i=0; i<root[tidx].tot_nodes; i++) {
			cidx = root[tidx].nodes[i];
			if(*dataset == cidx) {
				flag = 1;
				dataset += 1;
				tidx = cidx;
				break;
			}
		}

		if (flag == 0) {
			dataset += 1;
		}
	} while(1);
	ac = get_monotonic_time();

	sb_time_sum += (ac - bc);
	sb_test_count++;

#ifdef SMOKE_BOMB_ENABLE
	asm volatile("nop"); asm volatile("nop");
	if (eva == 0) {
	    get_current_pc(eva);
	}

    if (sva && eva && sb_init_flag == 1) {
        sb_ret = smoke_bomb_exit(sva, eva, dec_tree, sizeof(pre_node_t) * 12, sched_policy, sched_prio);
        if (sb_ret) {
            printf("smoke_bomb_exit error : %d\n", sb_ret);
            //exit(-1);
        } else {
            sb_init_flag = 0;
        }
    }
#endif

	return winvalue;
}

static int _test_prestored_tree( pre_node_t *node, char **data)
{
	int *dataset = NULL;
	int class;
	unsigned int dataset_sz = 0, i;

	dataset = convert_str_data_to_int(data, 5, 1);
	if(!dataset)
		return -1;

	/* traverse prestored tree */
	class = _traverse_prestored_tree(dec_tree, 31, dataset);
	
	free(dataset);
	return class;
}

int test_prestored_tree ( void )
{
	return _test_prestored_tree(dec_tree, testset);
}

int test_prestored_tree_with_data(char **data)
{
	return _test_prestored_tree(dec_tree, data);
}

/*
	Seconda scansione dell'albero di decisioni per la raccolta delle
	regole per ogni classe
*/
void scanrules( node_t *node, long class_id, long *depth, long *path, long maxdepth, long *table, long *tid )
{
	int j, i;

	/*
	if (node->winvalue == -1) {
		printf("==== node info ====\n");
		printf("node : %lx\n", node);
		printf("tot_attrib : %ld\n", node->tot_attrib);
		printf("avail_attrib : %lx\n", node->avail_attrib);
		printf("tot_samples : %ld\n", node->tot_samples);
		printf("samples : %lx\n", node->samples);
		printf("tot_nodes : %ld\n", node->tot_nodes);

		for(i=0; i<5; i++) {
			printf("%d, ", node->samples[i]);
		}
		printf("\n");
	}*/

	if( node != NULL )
	{
		*depth += 1;

		// aggiorno il path corrente
		*( path + ( *depth - 1 ) ) = node->winvalue;

		// e' l'utlimo nodo foglia del ramo
		if( node->tot_nodes == 0 && node->winvalue == class_id )
		{
			for( i = 0; i < *(depth)-1; i++ )
				*( table + ((*tid)*maxdepth) + i ) = path[ i ];
				*(tid)	+=1;
		}

		j = 0;
		while( j < node->tot_nodes )
		{
			scanrules( node->nodes+j, class_id, depth, path, maxdepth, table, tid );

			*depth -= 1;
			++j;
		}
	}
}
/*
	Estrazione delle regole contenuto nell'albero di decisioni
*/
void explain_rules( node_t *node, long cols, struct dsinfo_t *info, char **titles, long maxdepth, long maxrules )
{
	struct dsinfo_t 	*infoptr 		= info;
	struct dsinfo_t 	*infoptr2 		= NULL;
	long				*rules_table	= NULL;
	long				tableins_id		= 0;
	long				rulestable_sz	= 0;
	long				*temp_path		= NULL;
	long				attrb			= 0;
	long				attrb_id		= 0;
	long				*attrb_name		= 0;
	long				depth			= 0;
	long				i, j, k;

	// allocazione memoria per contenere le regole
	rulestable_sz 	= sizeof( long ) * maxdepth * maxrules;
	rules_table 	= malloc( rulestable_sz );
	temp_path 		= malloc( sizeof( long ) * maxdepth );

	printf( "Regole trovate:\n\n");
	while( infoptr->next != NULL )
	{
		if( infoptr->column == ( cols - 1 ) )
		{
			printf( "Classe %s\n", infoptr->name );

			i = 0;
			while( i < ( maxdepth * maxrules ) )
			{
				*( rules_table + i ) = -1;
				++i;
			}

			for( i = 0; i < maxdepth; i++ )	temp_path[ i ] = -1;
			depth 		= 0;
			tableins_id = 0;
			scanrules( node, infoptr->value, &depth, temp_path, maxdepth, rules_table, &tableins_id );


			/*
				Classe (4): NO
								-1  0  2 -1
								-1  8  5 -1
								-1 -1 -1 -1
								-1 -1 -1 -1
								-1 -1 -1 -1
				Classe (7): YES
								-1  0 11 -1
								-1  6 -1 -1
								-1  8  3 -1
								-1 -1 -1 -1
								-1 -1 -1 -1
			*/
			// stampa le regole trovate per la classe corrente
			printf("\t\t");
			for( i = 0; i < maxrules; i++ )
			{
				for( j = 0; j < (maxdepth-1); j++ )
				{
					attrb 		= *( rules_table + i*maxdepth + j );
					if( attrb >= 0 )
					{
						attrb_id 	= 0;
						infoptr2 	= info;
						while( infoptr2 != NULL )
						{
							if( attrb == infoptr2->value )
							{
								attrb_id 	= infoptr2->column;
								attrb_name 	= infoptr2->name;
								break;
							}
							infoptr2 = infoptr2->next;
						}
						printf( "attr %s = %s ", *( titles + attrb_id ), attrb_name );
						if( *( rules_table + i*maxdepth + j+1 ) >= 0 )
							printf( "and " );
						else
							printf( "\n\t\t" );
					}
				}
			}
			printf("\n");

		}
		infoptr = infoptr->next;
	}

	free( temp_path );
	free( rules_table );
}

/*
	Calcolo entropia porzione di samples
	- data: 		puntatore all'intero DataBase
	- cols:			numero di colonne DB (attributi + classi)
	- sample:		vettore contenente gli indici dei samples da analizzare
	- totsamples:	totale samples da analizzare
	- info:			informazioni su classi/atttributi
*/
double calc_entropy_set( long *data, long cols, long *samples, long totsamples, struct dsinfo_t *info )
{
	double 			entropy		= 0;
	double				part		= 0;
	long				total		= 0;
	struct dsinfo_t  	*infoptr	= NULL;
	long				j;

	// cerco all'interno della infolist gli indici delle classi
	infoptr = info;
	while( infoptr != NULL )
	{
		// quando trovo una classe...
		if( infoptr->column == ( cols - 1 ) )
		{
			// ne calcolo l'entropia sulla porzione di database indicata da samples
			// samples contiene gli indici dei sample da analizzare percui
			// data[ samples[ j ]*cols + cols - 1 ] contiene il valore della classe (ultima colonna)
			// delle elemento del DB con indice indicato da samples[ j ]

			total = 0;
			for( j = 0; j < totsamples; j++ )
				if( data[ samples[ j ]*cols + cols - 1 ] == infoptr->value ) ++total;

			// calcolo il rapporto su cui eseguire la formula
			if( total > 0 && totsamples > 0 )
			{
				part	= (double)total / (double)totsamples;

				// sommo all'entropia totale l'entropia per questa classe secondo
				// la formula	Entropy = -p(I) log2( p(I) )
				entropy += ( -part * log2(part) );
			}
		}
		// continuo la ricerca nelle info
		infoptr = infoptr->next;
	}

	return entropy;
}


/*
	Calcolo Info gain per un attributo
*/
double calc_attrib_gain( long *data, long cols, long *samples, long totsamples, struct dsinfo_t *info, long attrib )
{
	struct dsinfo_t  	*infoptr 		= NULL;
	long				tot_attribtype 	= 0;
	long				tot_classtype	= 0;
	double 			gain 			= 0;
	double				vpcgain			= 0;
	double				part			= 0;
	long				size			= 0;
	long				attrvalue		= 0;
	long				*classlist		= NULL;
	long				i = 0, j, k;

	struct vpc_t
	{
		long			class_id;
		long			tot_found;
	};

	struct gdata_t
	{
		long			value;
		long			tot_found;
		struct vpc_t	*vpc;
	};
	struct gdata_t		*gdata, *gdataptr;
	struct	vpc_t		*vpcptr;

	// Calcolo totale valori possibili per attributo e classi
	infoptr = info;
	while( infoptr != NULL )
	{
		// conteggio totale valori possibili per l'attributo
		if( infoptr->column == attrib ) 		++tot_attribtype;
		// conteggio totale valori possibili per le classi
		if( infoptr->column == ( cols - 1 ) ) 	++tot_classtype;
		// proseguo la ricerca
		infoptr = infoptr->next;
	}

	// classlist diventa un vettore contenente tutte le possibili classi
	classlist = malloc( sizeof( long ) * tot_classtype );
	infoptr = info , i = 0;
	while( infoptr != NULL )
	{
		if( infoptr->column == ( cols - 1 ) ) *( classlist + i++ ) = infoptr->value;
		infoptr = infoptr->next;
	}

	// allocazione memoria per le strutture per ogni tipo di valore dell'attributo
	size 	= sizeof( struct gdata_t ) * tot_attribtype;
	gdata 	= malloc( size );
	memset( gdata, 0, size );

	// inizializzazione struttura per ogni valore dell'attributo
	i = 0, infoptr = info;
	while( infoptr != NULL )
	{
		if( infoptr->column == attrib )
		{
			gdataptr 				= gdata + i;
			gdataptr->value 		= infoptr->value;
			gdataptr->tot_found 	= 0;

			size = sizeof( struct vpc_t ) * tot_classtype;
			gdataptr->vpc 			= malloc( size );

			for( j = 0; j < tot_classtype; j++ )
			{
				vpcptr 				= gdataptr->vpc + j;
				vpcptr->class_id	= *( classlist + j );
				vpcptr->tot_found	= 0;
			}
			++i;
		}
		// proseguo la ricerca
		infoptr = infoptr->next;
	}

	// raccolta dati dai samples riguardo al numero di valori per ogni
	// attributo; per ogni valore viene calcolato inoltre quanti corrispondono
	// a una classe piuttosto che ad un'altra
	for( i = 0; i < totsamples; i++ )
	{
		for( j = 0; j < tot_attribtype; j++ )
		{

			gdataptr = gdata + j;
			if( gdataptr->value == data[ samples[ i ]*cols + attrib ] )
			{
				gdataptr->tot_found += 1;
				for( k = 0; k < tot_classtype; k++ )
				{
					vpcptr = gdataptr->vpc;
					if( data[ samples[ i ]*cols + cols - 1 ] == ( vpcptr+k )->class_id )
							( vpcptr+k )->tot_found += 1;
				}
			}
		}
	}

	// calcolo information gain
	for( i = 0; i < tot_attribtype; i++ )
	{
		gdataptr 	= gdata + i;
		vpcgain		= 0;

		for( j = 0; j < tot_classtype; j++ )
		{
			vpcptr 	= 	gdataptr->vpc + j;
			if( vpcptr->tot_found > 0 && gdataptr->tot_found > 0 )
			{
				part	= 	(double)vpcptr->tot_found / (double)gdataptr->tot_found;
				vpcgain +=	( -( part ) * log2( part ) );
			}
 		}
		if( gdataptr->tot_found > 0 && totsamples > 0 )
		{
			part	= (double) gdataptr->tot_found / (double) totsamples;
			gain 	+= ( -( part ) * vpcgain );
		}
	}

	// Libera tutta la memoria allocata
	for( i = 0; i < tot_attribtype; i++ )
	{
		gdataptr = gdata + i;
		free( gdataptr->vpc );
	}
	free( gdata );
	free( classlist );

	return 	gain;
}

/*
	Creazione nodi albero
*/
void create_leaves( node_t *node, long *data, long cols, long rows, struct dsinfo_t *info )
{

	struct dsinfo_t  	*infoptr 		= NULL;
	double 			entropy_set 	= 0;
	double				*gains			= NULL;
	double				max_gain		= 0;
	long				max_gain_id		= 0;
	long				gbuf_sz			= 0;
	long				max_attr_values	= 0;
	long				tot_new_samples	= 0;
	long				tot_avattrib	= 0;
	long				*sampleptr		= NULL;
	node_t				*node_ptr		= NULL;
	node_t				*new_node		= NULL;
	long				j, i;

	struct smplid_t
	{
		long 				value;
		struct smplid_t 	*next;
		struct smplid_t 	*prev;
	};

	struct smplid_t	*samplelist		= NULL;
	struct smplid_t	*samplelistptr	= NULL;
	struct smplid_t	*samplelistprv	= NULL;


	DEBUG( "Current node @ %p:\n", node );
	DEBUG( "\twinvalue        : %d\n", node->winvalue );
	DEBUG( "\ttot_samples     : %d\n", node->tot_samples );
	DEBUG( "\tsamples         : " );
	for( i = 0; i < node->tot_samples; i++ )
		DEBUG( "%-2d ", node->samples[ i ] );
	DEBUG( "\n\ttot_attrib      : %d (%d %d %d %d )\n", node->tot_attrib, node->avail_attrib[0],node->avail_attrib[1],node->avail_attrib[2],node->avail_attrib[3] );
	DEBUG( "\ttot_nodes       : %d\n", node->tot_nodes );
	DEBUG( "\tnodes           @ %p\n", node->nodes );


	// calcolo entropia della parte di samples da esaminare
	entropy_set = calc_entropy_set( data, cols, node->samples, node->tot_samples, info );

	DEBUG( "Entropy set = %3.6f\n", entropy_set );
	// Il valore di entropy_set e' fondamentale per proseguire o meno nella crezione
	// dei rami e dei nodi foglia. Se il suo valore e' 0 significa che gli elementi
	// esaminati sono prerfettamente classificati, se il suo valore e' significa che
	// gli elementi non hanno regole, sono totalmente casuali.
	// Se invece il valore e' compreso tra 0 e 1 proseguo e calcolo il Gain per ogni
	// attributo disponibile..
	if( entropy_set == 0.000f )
	{

		node->nodes 				= malloc( sizeof( node_t ) );
		node->tot_nodes				= 1;
		node->nodes->tot_nodes 		= 0;
		node->nodes->winvalue 		= data[ node->samples[ 0 ] * cols + cols - 1 ];

		node->nodes->tot_attrib		= 0;
		node->nodes->avail_attrib	= NULL;
		node->nodes->tot_samples	= 0;
		node->nodes->samples		= NULL;
		node->nodes->nodes			= NULL;

		DEBUG( "\t\t\tNodo Terminale @ %p:\n", node->nodes );
		DEBUG( "\t\t\twinvalue        : %d\n", node->nodes->winvalue );
		DEBUG( "\t\t\ttot_samples     : %d\n", node->nodes->tot_samples );
		DEBUG( "\t\t\ttot_attrib      : %d\n", node->nodes->tot_attrib );
		DEBUG( "\t\t\ttot_nodes       : %d\n", node->nodes->tot_nodes );
		DEBUG( "\t\t\tnodes           @ %p\n", node->nodes->nodes );
	}
	else if( entropy_set == 1 )
	{
		// Dati totalmente casuali, nessuna regola
	}
	else
	{
		// calcola il totale degli attributi su cui calcolare...
		tot_avattrib = 0;
		for( j = 0; j < ( cols - 1 ); j++ )
			if( node->avail_attrib[ j ] == 1 ) tot_avattrib += 1;

		DEBUG( "\tCalcolo entropia per ogni attributo ( tot. disponibili %d )\n", tot_avattrib );
		// se c'e' piu' di un attributo disponibile
		if( tot_avattrib > 0 )
		{

			// allocazione memoria per il buffer di dimensione n. attributi
			// all'interno del buffer
			gains = malloc( sizeof( double ) * ( cols - 1 ) );
			for( i = 0; i < ( cols - 1 ); i++ ) gains[ i ] = 0;

			for( j = 0; j < ( cols - 1 ); j++ )
				if( node->avail_attrib[ j ] == 1 )
				{
					gains[ j ] = entropy_set + calc_attrib_gain( data, cols, node->samples, node->tot_samples, info, j );
					DEBUG( "\tInfo Gain per attributo %d = %3.3f\n", j, gains[ j ] );
				}
			// cerca il valore piu' alto...
			for( j = 0; j < ( cols - 1 ); j++ )
				if( gains[ j ] > max_gain )
				{
					max_gain	= gains[ j ];
					max_gain_id = j;
				}

			// calcola il numero massimo possibile di valori per l'attributo vincente
			max_attr_values = 0;
			infoptr 		= info;
			while( infoptr != NULL )
			{
				if( infoptr->column == max_gain_id ) ++max_attr_values;
				infoptr = infoptr->next;
			}
			DEBUG( "\tL'attributo %d ha il massimo IG (%3.3f) e %d tipi di valori\n", max_gain_id, max_gain, max_attr_values );

			// crea i nodi per ogni valore possibile dell'attributo
			// il numero dei nodi e' pari a tutti i valori possibili per l'attributo
			node->nodes 	= ( node_t* ) malloc( sizeof( node_t ) * max_attr_values );
			node->tot_nodes = max_attr_values;
			DEBUG( "\tAllocazione memoria per %d nodi @ %p\n", max_attr_values, node->nodes );

			infoptr 		= info;
			j = 0;
			while( infoptr != NULL )
			{
				if( infoptr->column == max_gain_id )
				{
					DEBUG( "\t\tImpostazione nodo per valore %d dell'attributo %d\n", infoptr->value, max_gain_id );

					node_ptr 	= node->nodes;
					node_ptr 	+= j;
					DEBUG( "\t\t\tnode_ptr = %p ( j = %d )\n", node_ptr, j );

					tot_new_samples = 0;
					// cercare nei sample del DB indicati da node->samples tutti
					// quelli che nella colonna indicata da max_gain_id hanno il valore indicato
					// da infoptr->value, calcolarne il totale e metterlo in tot_samples
					// creare un vettore della dimensione di tot_samples e assegnarlo a node_ptr->samples
					for( i = 0; i < node->tot_samples; i++ )
					{
						if( data[ node->samples[ i ] * cols + max_gain_id ] == infoptr->value )
						{
							if( samplelist == NULL )
							{
								samplelist 				= malloc( sizeof( struct smplid_t ) );
								samplelist->value 		= node->samples[ i ];
								samplelist->next		= NULL;
								samplelist->prev		= NULL;
							}
							else
							{
								samplelistptr				= samplelist;
								while( samplelistptr->next != NULL ) samplelistptr = samplelistptr->next;
								samplelistptr->next			= malloc( sizeof( struct smplid_t ) );
								samplelistptr->next->prev 	= samplelistptr;
								samplelistptr 				= samplelistptr->next;
								samplelistptr->value 		= node->samples[ i ];
								samplelistptr->next			= NULL;
							}
							tot_new_samples += 1;
						}
					}

					node_ptr->winvalue		= infoptr->value;
					node_ptr->tot_nodes 	= 0;
					node_ptr->tot_samples 	= tot_new_samples;
					node_ptr->samples		= malloc( sizeof( long ) * tot_new_samples );
					sampleptr				= node_ptr->samples;

					samplelistptr			= samplelist;
					while( samplelistptr != NULL )
					{
						*( sampleptr++ ) 	= samplelistptr->value;
						samplelistptr 		= samplelistptr->next;
					}
					// una volta inseriti gli indici dei nuovi sample in test nel vettore
					// puntato da node_ptr->samples posso distruggere la lista temporanea
					samplelistptr			= samplelist;
					samplelistprv			= samplelist;
					while( samplelistptr != NULL )
					{
						samplelistprv = samplelistptr->next;
						free( samplelistptr );
						samplelistptr = samplelistprv;
					}
					samplelist = NULL;

					node_ptr->tot_attrib 	= ( cols - 1 );
					node_ptr->avail_attrib	= malloc( sizeof( long ) * ( cols - 1 ) );

					for( i = 0; i < cols-1; i++ )
						node_ptr->avail_attrib[ i ] = node->avail_attrib[ i ];
					node_ptr->avail_attrib[ max_gain_id ] = 0;

					DEBUG( "\t\t\tnode_ptr->winvalue    : %d\n", node_ptr->winvalue );
					DEBUG( "\t\t\tnode_ptr->tot_samples : %d\n", node_ptr->tot_samples );
					DEBUG( "\t\t\tnode_ptr->samples     : %p\n", node_ptr->samples );


					// creazione ricorsiva dei nodi foglia
					if( node_ptr->tot_samples > 0 ) create_leaves( node_ptr, data, cols, rows, info );


					++j;
				}
				infoptr = infoptr->next;
			}
			free( gains );
		}
		else
		{
			node->nodes 				= malloc( sizeof( node_t ) );
			node->tot_nodes				= 1;
			node->nodes->tot_nodes 		= 0;
			node->nodes->winvalue 		= data[ node->samples[ 0 ] * cols + cols - 1 ];

			node->nodes->tot_attrib		= 0;
			node->nodes->avail_attrib	= NULL;
			node->nodes->tot_samples	= 0;
			node->nodes->samples		= NULL;
			node->nodes->nodes			= NULL;

			DEBUG( "\t\t\tNodo Terminale @ %p:\n", node->nodes );
			DEBUG( "\t\t\twinvalue        : %d\n", node->nodes->winvalue );
			DEBUG( "\t\t\ttot_samples     : %d\n", node->nodes->tot_samples );
			DEBUG( "\t\t\ttot_attrib      : %d\n", node->nodes->tot_attrib );
			DEBUG( "\t\t\ttot_nodes       : %d\n", node->nodes->tot_nodes );
			DEBUG( "\t\t\tnodes           @ %p\n", node->nodes->nodes );
		}
	}
}

/*
	Creazione regole
*/
int id3tree_create( char **data, long cols, long rows, ... )
{
    long				*dataset		= NULL;
	unsigned long		dataset_sz 		= 0;
	struct dsinfo_t  	*infolist		= NULL;
	struct dsinfo_t  	*insptr 		= NULL;
	struct dsinfo_t  	*prvptr 		= NULL;
	struct dsinfo_t  	*prvass 		= NULL;
	char				label_found		= 0;
	char				infolisterror	= 0;
	long				string_id		= 0;
	long				assign_id		= 0;
	node_t		*root			= NULL;

	va_list				llistptr;
	char				**cols_titles	= NULL;
	char				**ctptr			= NULL;
	char				*label;
	long				totlabels		= 0;

	long				tree_max_depth	= 0;
	long				tree_max_rules	= 0;


	int					result			= 0;
	int 				i = 0, j = 0, col = 0;

	DEBUG( "ID3 Init: cols = %d rows = %d dataset %p\n", cols, rows, data );

	do {

		// Riempimento della lista di puntatori a stringa contenenti
		// le etichette (titoli) per ogni attributo
		va_start( llistptr, rows );
		do {
			label = va_arg( llistptr, int );
			if( label != NULL ) totlabels += 1;
		} while( label != NULL );
		va_end( llistptr );

		cols_titles = malloc( sizeof( char* )*totlabels );
		// controllo se posso allocare memoria per le labels
		if( cols_titles == NULL )
		{
			result = -1;
			break;
		}
		// riempio il vettore con dimensione pari al numero di colonne del database appena creato
		ctptr = cols_titles;
		va_start( llistptr, rows );
		for( i = 0; i < totlabels; i++ )
		{
			label 	= va_arg( llistptr, int );
			*ctptr 	= label;
			++ctptr;
		}
		va_end( llistptr );

		DEBUG( "Columns labels:\n" );
		for( i = 0; i < totlabels; i++ )
			DEBUG( "Label %3d = %s\n", i, cols_titles[ i ] );


		// Essendo la comparazione di valori long piu' veloce rispetto alla
		// comparazione di stringhe l'intero data set costituito da stringhe viene
		// convertito assegnando un indice univoco ad ogni stringa
		// calcolo quantita' di memoria necessaria per tabella di conversione
		dataset_sz = sizeof( long ) * cols * rows;

		// Allocazione memoria per la conversione stringa->value
		if( ( dataset = malloc( dataset_sz ) ) == NULL )
		{
			result = -2;
			break;
		}
		// azzero la tabella completamente
		memset( dataset, 0, dataset_sz );


		// L'intero data set (in stringa) viene scorso completamente per
		// recuperare tutte le informazioni necessarie ai calcoli per la creazione
		// dell'albero delle regole
		i = 0, col = 0;
		while( i < (cols*rows) )
		{
			// se infolist e' NULL significa che l'elemento va ovviamente inserito nella lista
			insptr = NULL;
			if( infolist == NULL )
			{
				infolist 	= malloc( sizeof( struct dsinfo_t ) );
				if( infolist == NULL )
				{
					infolisterror = 1;
					break;
				}
				// TODO inserire controllo valore infolist
				insptr		= infolist;
				prvass		= NULL;
			}
			else
			{
				// altrimenti viene cercato fra tutti quelli trovati finora. se non compare
				// nella lista viene aggiunto
				insptr 		= infolist;
				prvptr		= infolist;
				label_found	= 0;
				do {
					if( !strcmp( insptr->name, data[ i ] ) )
					{
						label_found = 1;
						assign_id	= insptr->value;
						insptr		= NULL;
						break;
					}
					prvptr	= insptr;
					insptr 	= insptr->next;
				} while( insptr != NULL );

				if( label_found == 0 )
				{
					prvptr->next 	= malloc( sizeof( struct dsinfo_t ) );
					if( prvptr->next == NULL )
					{
						infolisterror = 1;
						break;
					}
					insptr			= prvptr->next;
					prvass			= prvptr;
				}
			}
			// se insptr e' diverso da NULL significa che punta a una zona di memoria
			// gia' allocata per poter contenere i dati sulla nuova etichetta
			if( insptr != NULL )
			{
				assign_id		= string_id;
				insptr->name 	= malloc( sizeof( char )*strlen( data[ i ] ) + 1 );
				if( insptr->name == NULL )
				{
					infolisterror = 1;
					break;
				}

				sprintf( insptr->name, data[ i ] );
				insptr->value	= string_id++;
				insptr->column	= col;
				insptr->next	= NULL;
				insptr->prev	= prvass;
			}

			// aggiorno il corrispondente long nella tabella di conversione string->long
			dataset[ i ] = assign_id;


			// la variabile col tiene conto della colonna corrente all'interno del dataset
			// in caso di accodamento tiene traccia dell'attributo/classe a cui appartiene l'elemento
			if( ++col >= cols ) col = 0;
			// incremento indice di scorrimento elementi nel dataset
			i += 1;
		}
		// controllo se ci sono stati errori di allocazione memoria durante la creazione del dataset
		if( infolisterror )
		{
			result = -3;
			break;
		}


		// creazione del nodo radice: da qui parte la creazione dell'intero albero
		if( ( root = ( node_t* ) malloc( sizeof( node_t ) ) ) == NULL )
		{
			result = -4;
			break;
		}
		// essendo alla radice il set di samples da esaminare e' l'intero albero
		root->tot_samples	= rows;
		// creo un vettore contenente gli indici ( da 0 a row - 1 ) di tutti i samples da esaminare
		if( ( root->samples = malloc( sizeof( long ) * rows ) ) == NULL )
		{
			result = -5;
			break;
		}
		// il nodo radice contiene gli indici di tutti i samples del database
		for( j = 0; j < rows; j++ ) root->samples[ j ] = j;

		// imposto tutti gli attributi possibili ( tutte le colonne meno una, quella delle classi )
		root->tot_attrib = ( cols - 1 );
		// tutti gli attributi ( cols - 1 ) devono essere presi in considerazione
		if( ( root->avail_attrib	= malloc( sizeof( long ) * ( cols - 1 ) ) ) == NULL )
		{
			result = -6;
			break;
		}
		// al nodo radice tutti gli attributi sono da controllare
		for( j = 0; j < ( cols - 1 ); j++ ) root->avail_attrib[ j ] = 1;

		// valore (-1) che identifica il nodo radice, inoltre il nodo non ha (all'inizio) sotto nodi
		root->winvalue		= -1;
		root->tot_nodes		= 0;

		DEBUG( "Root node @ %p:\n", root );
		DEBUG( "\twinvalue        : %d\n", root->winvalue );
		DEBUG( "\ttot_samples     : %d\n", root->tot_samples );
		DEBUG( "\tsamples         : " );
		for( i = 0; i < root->tot_samples; i++ )
			DEBUG( "%2d ", root->samples[ i ] );
		DEBUG( "\n\ttot_attrib      : %d (%d %d %d %d )\n", root->tot_attrib, root->avail_attrib[0],root->avail_attrib[1],root->avail_attrib[2],root->avail_attrib[3] );
		DEBUG( "\ttot_nodes       : %d\n", root->tot_nodes );
		DEBUG( "\tnodes           @ %p\n", root->nodes );


		// creazione albero e nodi foglia
		create_leaves( root, dataset, cols, rows, infolist );

		// visualizzazione albero
		scantree( root, &tree_max_depth, &tree_max_rules );

		// spiegazione delle regole
		explain_rules( root, cols, infolist, cols_titles, tree_max_depth, tree_max_rules );

		/* [JB] test */
		//convert_str_data_to_long(testset, cols, 1, infolist);
		//create_prestored_tree(root, infolist);
		//test_prestored_tree();

	} while( 0 );

	// TODO Libero memoria albero

	// Libero memoria info dataset
	insptr = infolist;
	while( insptr != NULL )
	{
		prvass = insptr->next;
		free( insptr );
		insptr = prvass;
	}
	// Libero memoria allocata per la tabella conversione stringa->valore
	if( dataset != NULL ) 	free( dataset );
	// Libero memoria allocata per etichette attributi/classi
	if( cols_titles != NULL ) free( cols_titles );

	return result;
}


