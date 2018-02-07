/*
 *	sync one round attack
 *
 *	Copyright (C) 2017  Jinbum Park <jinb.park@samsung.com>
*/

#include "common.h"
#include <openssl/aes.h>
#include <unistd.h>
#include <sched.h>
#include <sb_api.h>

#ifdef __aarch64__
#define _SMOKE_BOMB_ARMV8
#else
#define _SMOKE_BOMB_ARMV7
#endif

#include "../../smoke-bomb/header.h"
#include <sb_api.h>

U8 plain[16] = {0x00,};
U8 enc[16] = {0x10,0x20,};
U8 real_key[16] = {0x9e,0x41,0x00,0x5f,0xd9,0xa0,0xc2,0x28,0xe7,0xb4,0x31,0x29,0x94,0xaa,0x4d,0xc7};
AES_KEY aes_key;

unsigned long test_count = 0;
unsigned long cycle_sum = 0;

void set_aes_key(void)
{
	if(AES_set_encrypt_key(real_key, 128, &aes_key) < 0) {
		printf("set_aes_key error\n");
		exit(-1);
	}
}
void aes_encrypt(U8 *in, U8 *out)
{
	unsigned long bc, ac;

	bc = get_cycle_count();
	AES_ecb_encrypt(in, out, &aes_key, AES_ENCRYPT);
	ac = get_cycle_count();
	
	cycle_sum += (ac - bc);
	test_count++;
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

void bind_cpu_all(void)
{
    unsigned long mask = 15;

    if(sched_setaffinity(0, sizeof(mask), (cpu_set_t*)&mask) < 0)
    {
        printf("sched_setaffinity error\n");
        return;
    }
}

#include <sched.h>
void __attribute__((optimize ("-O0"))) repeat_aes_encrypt(void)
{
	U8 tplain[16];
	U8 tout[16];
	U32 i;

	for (i=0; i<16; i++) {
		tplain[i] = rand() % 256;
	}

	while (1) {
		bind_cpu(1);
		AES_ecb_encrypt(tplain, tout, &aes_key, AES_ENCRYPT);
		bind_cpu_all();
		sched_yield();
	}
}

#include <time.h>
#include <stdint.h>
static inline uint64_t get_ms_time(void)
{
	struct timespec t1;
	clock_gettime(CLOCK_MONOTONIC, &t1);
	return (t1.tv_sec * 1000) + (t1.tv_nsec / 1000000);
}

void __attribute__((optimize ("-O0"))) aes_encrypt_ms(int test_count)
{
	U8 tplain[16];
	U8 tout[16];
	U32 i;
	uint64_t bc, ac;
	uint64_t time;
	unsigned int count;

	for (i=0; i<16; i++) {
		tplain[i] = rand() % 256;
	}

	for (i=0; i<test_count; i++) {	
		AES_ecb_encrypt(tplain, tout, &aes_key, AES_ENCRYPT);
	}
	//printf("test count : %d, time : %lld ms\n", test_count, ac - bc);

	sb_get_time(&time, &count);
	printf("test count : %d, time : %lld ns\n", count, time / count);
}

int main(int argc, char **argv)
{
	char msg[MSG_SIZE_MAX];
	struct shm_msg *client_msg;
	struct shm_msg *server_msg;
	void *addr;
	int fd;
	int option, test_count;

	if (argc > 1) {
		/* if argv[1] == 1, repeat aes encryption for benchmark */
		option = atoi(argv[1]);

		if (option == 1) {
			set_aes_key();
			repeat_aes_encrypt();
		}
		else if (option == 2) {
			test_count = atoi(argv[2]);
			
			set_aes_key();
			aes_encrypt_ms(test_count);
			return 0;
		}
	}
	
	set_aes_key();
	bind_cpu(2);
	smoke_bomb_init_pmu();

	/* create shm */
	if((fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, PERM_FILE)) == -1) {
		printf("shm_open error : %s\n", strerror(errno));
		return -1;
	}

	/* set size */
	if(ftruncate(fd, MSG_SIZE_MAX) == -1) {
		printf("ftruncate error : %s\n", strerror(errno));
		goto out;
	}

	/* mmap */
	addr = mmap(NULL, MSG_SIZE_MAX, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if(addr == MAP_FAILED) {
		printf("mmap error : %s\n", strerror(errno));
		goto out;
	}
	memset(addr, 0, MSG_SIZE_MAX);

	client_msg = (struct shm_msg *)((char*)addr + SHM_CLIENT_BUF_IDX);
	server_msg = (struct shm_msg *)((char*)addr + SHM_SERVER_BUF_IDX);

	printf("server is running\n");
	while(1) {
		/* read msg */
		while(1) {
			if(client_msg->status == 1) {
				memcpy(plain, client_msg->msg, client_msg->len);
				client_msg->status = 0;
				break;
			}
		}

		if(client_msg->len == sizeof(END_MSG)) {
			printf("end msg : %s\n", client_msg->msg);
			break;
		}

		/* encrypt */
		aes_encrypt(plain, enc);

		/* prepare msg */
		server_msg->status = 0;
		server_msg->len = sizeof(enc);

		/* send reply */
		memcpy(server_msg->msg, enc, sizeof(enc));
		server_msg->status = 1;
	}

	/* print performance */
	printf("performance : %ld cycles\n", cycle_sum / test_count);

out:
	/* destroy shm */
	if(munmap(addr, MSG_SIZE_MAX) == -1) {
		printf("munmap error : %s\n", strerror(errno));
	}

	if(close(fd) == -1) {
		printf("close error : %s\n", strerror(errno));
	}

	if(shm_unlink(SHM_NAME) == -1) {
		printf("shm_unlink error : %s\n", strerror(errno));
	}

	return 0;
}
