/*
 *	sync one round attack
 *
 *	Copyright (C) 2017  Jinbum Park <jinb.park@samsung.com>
*/

#include "common.h"
#include <id3.h>

#ifdef __aarch64__
#define _SMOKE_BOMB_ARMV8
#else
#define _SMOKE_BOMB_ARMV7
#endif

#include "../../smoke-bomb/header.h"

unsigned long test_count = 0;
unsigned long cycle_sum = 0;

char *testset1[] = 
{
	/*
	 * outlook, temperature, humidity, wind, play ball (class)
	 */
	"SUNNY", 	"MILD",  	"HIGH",    	"WEAK",		"NO",
};

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

#include <time.h>
#include <stdint.h>
static inline uint64_t get_ms_time(void)
{
    struct timespec t1;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    return (t1.tv_sec * 1000) + (t1.tv_nsec / 1000000);
}

void __attribute__((optimize ("-O0"))) test_ms(int test_count)
{
	int i;
	uint64_t time;
	unsigned int count;

	//bc = get_ms_time();
	for (i=0; i<test_count; i++) {
		test_prestored_tree_with_data(testset1);
	}
	//ac = get_ms_time();

	sb_get_time(&time, &count);
	printf("test count : %d, time : %lld ns\n", count, time / count);
}

int main(int argc, char **argv)
{
	char msg[MSG_SIZE_MAX];
	char reply_msg[] = "reply!!";
	struct shm_msg *client_msg;
	struct shm_msg *server_msg;
	void *addr;
	int fd;
	unsigned long ac, bc;
	int option, test_count;

	if (argc > 1) {
		option = atoi(argv[1]);

		if (option == 2) {
			test_count = atoi(argv[2]);
			test_ms(test_count);
			return 0;
		}
	}

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
				memcpy(msg, client_msg->msg, client_msg->len);
				client_msg->status = 0;
				break;
			}
		}

		if(client_msg->len == sizeof(END_MSG)) {
			printf("end msg : %s\n", client_msg->msg);
			break;
		}

		/* test decision tree!! */
		bc = get_cycle_count();
		test_prestored_tree_with_data(testset1);
		ac = get_cycle_count();
		
		cycle_sum += (ac - bc);
		test_count++;

		/* prepare msg */
		server_msg->status = 0;
		server_msg->len = sizeof(reply_msg);

		/* send reply */
		memcpy(server_msg->msg, reply_msg, sizeof(reply_msg));
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

	return 0;
}
