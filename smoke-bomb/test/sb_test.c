#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>

#include "../lib/sb_api.h"
#include "../header.h"

const unsigned int test_array[256] = {
	0, 30, 90, 11, 20, 50, 60,
};

void get_context_switches(long long *vol, long long *invol)
{
	struct rusage usage;
	int r;

	r = getrusage(RUSAGE_SELF, &usage);
	if (r) {
		printf("getrusage error : %d\n", r);
		return;
	}

	*vol = usage.ru_nvcsw;
	*invol = usage.ru_nivcsw;
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

void write_file(const char *name)
{
	FILE *fp;
	int fd;
	unsigned i;
	char str[] = "hello\n";
	char str2[] = "world\n";

	fp = fopen(name, "w");
	if (fp) {
		fd = fileno(fp);
		
		for (i=0; i<150000; i++) {
			fwrite(str, 1, strlen(str), fp);
			fwrite(str2, 1, strlen(str2), fp);
			
			if (i % 1000 == 0) {
				fflush(fp);

				/*
				 * fsync ensure writing data to disk.
				 * voluntary context switch happens when write-to-disk is triggered.
				 */
				fsync(fd);
			}
		}
		fclose(fp);
	}
}

void dummy_delay(void)
{
	unsigned i;
	
	for (i=0; i<99999999; i++) asm volatile ("nop");
}

unsigned int test_func(int idx)
{
	unsigned int *ptr;

	ptr = test_array + idx;
	return *ptr;
}

unsigned int test_func2(int idx)
{
	unsigned int *ptr;

	ptr = test_array + idx;
	return *ptr;
}

void test_with_smoke_bomb(int size, int idx)
{
	int i;
	int r;
	int sched_policy, sched_prio;
	unsigned int val;
	unsigned long bc, ac;
	long long vol1, vol2, invol1, invol2;

	val = test_func(idx);
	bc = get_cycle_count();
	val = test_func(idx);
	ac = get_cycle_count();
	printf("test_func : [LDR, cache-hit] : %ld, %ld cycles\n", val, ac - bc);	/* LDR case, cache-hit */

/*
	for (i=0; i<(size/4); i++) {
		printf("before test_func data : %08x\n", *((unsigned int *)test_func + i));
	}*/
	
	r = smoke_bomb_init((unsigned long)test_func, (unsigned long)test_func + size, &sched_policy, &sched_prio);
	if (r) {
		printf("smoke_bomb_init error\n");
		return;
	}
	get_context_switches(&vol1, &invol1);
	smoke_bomb_print_cpuid();

/*
	for (i=0; i<(size/4); i++) {
		printf("after test_func data : %08x\n", *((unsigned int *)test_func + i));
	}*/

	bc = get_cycle_count();
	val = test_func(idx);
	ac = get_cycle_count();
	printf("test_func [LDRFLUSH, cache-miss] : %ld, %ld cycles\n", val, ac - bc);	/* LDRFLUSH case, cache-miss */
	smoke_bomb_print_cpuid();

	/* If LDRFLUSH works well, test_func2() face cache-miss */
	bc = get_cycle_count();
	val = test_func2(idx);
	ac = get_cycle_count();
	printf("test_func2 [LDR, cache-miss] : %ld, %ld cycles\n", val, ac - bc);	/* LDR case, cache-miss */
	smoke_bomb_print_cpuid();

	/* this dummy loop is to measure how many context switch happens */
	dummy_delay();
	smoke_bomb_print_cpuid();

	/* File I/O test */
	write_file("./hello.dat");
	smoke_bomb_print_cpuid();

	get_context_switches(&vol2, &invol2);
	r = smoke_bomb_exit((unsigned long)test_func, (unsigned long)test_func + size, sched_policy, sched_prio);
	if (r) {
		printf("smoke_bomb_exit error\n");
		return;
	}

	printf("voluntary context switches : %lld\n", vol2 - vol1);
	printf("involuntary context switches : %lld\n", invol2 - invol1);
}

void test_without_smoke_bomb(int size, int idx)
{
	unsigned int val;
	
	printf("test_func data : %08x\n", *((unsigned int *)test_func));
	val = test_func(idx);
}

int main(int argc, char **argv)
{
	int cmd;
	int size;
	int idx;

	if (argc != 4) {
		printf("USAGE:  ./sb_test <cmd> <size> <idx>\n");
		printf("Example:  ./sb_test 1 64 64\n");
		return 0;
	}

	cmd = atoi(argv[1]);
	size = atoi(argv[2]);
	idx = atoi(argv[3]);

	//bind_cpu(1);
	smoke_bomb_init_pmu();

	if (cmd == 1)
		test_with_smoke_bomb(size, idx);
	else
		test_without_smoke_bomb(size, idx);

	return 0;
}
