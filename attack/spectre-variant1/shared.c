#include "common.h"

const struct arr_type probe_array[10] __attribute__((aligned (64))) = {
	[0] = {
		.val1 = 10,
		.val2 = 20,
	},
};

const unsigned int arr_bound1 = 0;
const unsigned int arr_bound2 = 10;

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

void sb_get_time(uint64_t *time, unsigned int *count)
{
	*time = sb_time_sum;
	*count = sb_test_count;
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

unsigned int get_probe_array_val1(int idx)
{
	unsigned int val;
	uint64_t bc, ac;
	
#ifdef SMOKE_BOMB_ENABLE
		int sb_ret = 0;
		int sched_policy = -1, sched_prio = -1;
	
		if (sva && eva && sb_init_flag == 0) {
			sb_ret = smoke_bomb_init(sva, eva, probe_array, sizeof(probe_array), &sched_policy, &sched_prio);
			if (sb_ret) {
				printf("smoke_bomb_init error : %d\n", sb_ret);
				//exit(-1);
			} else {
				sb_init_flag = 1;
			}
		}
	
		asm volatile("nop");
		if (sva == 0) {
			get_current_pc(sva);
		}
#endif

	bc = get_monotonic_time();
	val = probe_array[idx].val1;
	ac = get_monotonic_time();

	sb_time_sum += (ac - bc);
	sb_test_count++;

#ifdef SMOKE_BOMB_ENABLE
	asm volatile("nop");
	if (eva == 0) {
	    get_current_pc(eva);
	}

    if (sva && eva && sb_init_flag == 1) {
        sb_ret = smoke_bomb_exit(sva, eva, probe_array, sizeof(probe_array), sched_policy, sched_prio);
        if (sb_ret) {
            printf("smoke_bomb_exit error : %d\n", sb_ret);
            //exit(-1);
        } else {
            sb_init_flag = 0;
        }
    }
#endif

	return val;

}

