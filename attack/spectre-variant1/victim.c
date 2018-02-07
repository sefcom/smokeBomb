#include <stdio.h>
#include <libflush.h>
#include "common.h"

#ifdef __aarch64__
#define _SMOKE_BOMB_ARMV8
#else
#define _SMOKE_BOMB_ARMV7
#endif

#include "../../smoke-bomb/header.h"
#include <sb_api.h>

unsigned long test_count = 0;
unsigned long cycle_sum = 0;

const int secret = 3;
const int arr[10] = {0,1,2,3,4,5,6,7,8,9,};
const int secret2 = 2;
libflush_session_t *session;

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

static inline void flush_arr_bound(void)
{
	libflush_flush(session, &arr_bound1);
	libflush_flush(session, &arr_bound2);
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

void victim_func(int idx)
{
	unsigned int val;
	
	if (idx >= arr_bound1 && idx < arr_bound2) {
		val = get_probe_array_val1(arr[idx]);
	}

	if (val == 999)
		printf("val 999\n");
}

#include <time.h>
#include <stdint.h>
static inline uint64_t get_ms_time(void)
{
    struct timespec t1;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    return (t1.tv_sec * 1000) + (t1.tv_nsec / 1000000);
}

void sb_get_time(uint64_t *time, unsigned int *count);

void __attribute__((optimize ("-O0"))) victim_ms(int test_count)
{
	int i;
	uint64_t bc, ac;
	uint64_t time;
	unsigned int count;

	//bc = get_ms_time();
	for (i=0; i<test_count; i++)
		victim_func(0);
	//ac = get_ms_time();

	sb_get_time(&time, &count);
	printf("test count : %d, time : %lld ns\n", count, time / count);
}

int main(int argc, char **argv)
{
	int input_idx;
	unsigned int val;
	unsigned long bc, ac;
	
	char reply_msg[] = "reply!!";
	struct shm_msg *client_msg;
	struct shm_msg *server_msg;
	void *addr;
	int fd;
	int option, test_count;

	if (argc > 1) {
		option = atoi(argv[1]);

		if (option == 2) {
			test_count = atoi(argv[2]);
			victim_ms(test_count);
			return 0;
		}
	}

	init_libflush();
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
				memcpy(&input_idx, client_msg->msg, sizeof(input_idx));
				client_msg->status = 0;
				break;
			}
		}

		flush_arr_bound();
		if(client_msg->len == sizeof(END_MSG)) {
			printf("end msg : %s\n", client_msg->msg);
			break;
		}

		/* test!! */
		bc = get_cycle_count();
		victim_func(input_idx);
		ac = get_cycle_count();

		cycle_sum += (ac - bc);
		test_count++;

		/* prepare msg */
		server_msg->status = 0;
		server_msg->len = sizeof(reply_msg);

		/* send reply */
		memcpy(server_msg->msg, &val, sizeof(val));
		server_msg->status = 1;
	}

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

	finalize_libflush();
	return 0;
}

