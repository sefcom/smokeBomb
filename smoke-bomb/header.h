#ifndef _SMOKE_BOMB_HEADER_H
#define _SMOKE_BOMB_HEADER_H

#ifdef _SMOKE_BOMB_LKM

#include <linux/kernel.h>

#ifdef _SMOKE_BOMB_DEBUG
#define sb_pr_info(...) pr_info(__VA_ARGS__)
#define sb_pr_err(...) pr_err(__VA_ARGS__)
#else /* !_SMOKE_BOMB_DEBUG */
#define sb_pr_info(...)
#define sb_pr_err(...) pr_err(__VA_ARGS__)
#endif

#endif /* _SMOKE_BOMB_LKM */


#ifdef _SMOKE_BOMB_ARMV7

#ifdef _SMOKE_BOMB_LKM
#include "arm/cache.h"
#include "arm/insn.h"
#include "arm/patch.h"
#endif

static inline unsigned long get_cycle_count(void)
{
    unsigned long cycles;

    asm volatile ("isb\n");
    asm volatile ("dmb\n");
    asm volatile ("MRC p15, 0, %0, C9, C13, 0\n": "=r" (cycles));
    asm volatile ("isb\n");

    return cycles;
}
#else /* !_SMOKE_BOMB_ARMV7 */

#ifdef _SMOKE_BOMB_LKM
#include "arm64/cache.h"
#include "arm64/insn.h"
#include "arm64/patch.h"
#endif

static inline unsigned long get_cycle_count(void)
{
	unsigned long result = 0;
    asm volatile ("MRS %0, PMCCNTR_EL0" : "=r" (result));
    asm volatile ("ISB");
    asm volatile ("DSB SY");
    return result;
}

#endif /* _SMOKE_BOMB_ARMV7 */

struct smoke_bomb_cmd_arg {
	unsigned long sva;
	unsigned long eva;
	int sched_policy;
	int sched_prio;
}__attribute__((packed));

struct smoke_bomb_cmd {
	unsigned int cmd;
	struct smoke_bomb_cmd_arg arg;
}__attribute__((packed));

struct smoke_bomb_cmd_vector {
	unsigned int cmd;
	int (*func)(struct smoke_bomb_cmd_arg *);
}__attribute__((packed));

#define SMOKE_BOMB_CMD_INIT 0
#define SMOKE_BOMB_CMD_EXIT 1
#define SMOKE_BOMB_CMD_INIT_PMU 2
#define SMOKE_BOMB_CMD_PRINT_CPUID 3

#define SMOKE_BOMB_PROC_NAME "smoke_bomb"
#define SMOKE_BOMB_PROC_FULL_NAME "/proc/smoke_bomb"

#endif