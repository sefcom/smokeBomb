#ifndef _SMOKE_BOMB_API_H
#define _SMOKE_BOMB_API_H

#ifdef __cplusplus
extern "C" {
#endif

int smoke_bomb_init(unsigned long sva, unsigned long eva, int *sched_policy, int *sched_prio);
int smoke_bomb_exit(unsigned long sva, unsigned long eva, int sched_policy, int sched_prio);

/* api for debugging */
int smoke_bomb_init_pmu(void);
int smoke_bomb_print_cpuid(void);

#ifdef __cplusplus
}
#endif

#endif
