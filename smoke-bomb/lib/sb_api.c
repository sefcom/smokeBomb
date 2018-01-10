#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sched.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "sb_api.h"
#include "../header.h"

static int _smoke_bomb_write_to_lkm(char *fname, char *buf, unsigned int len)
{
    int ret = 0;
    int fd = 0;
    int size;

    if (!fname || !buf || !len)
    {
        printf("Input is NULL\n");
        return -1;
    }

    fd = open(fname, O_WRONLY, S_IXUSR | S_IROTH);
    if (fd < 0)
    {
        printf("fail to open : %s\n", fname);
        return -1;
    }

    size = write(fd, buf, len);
    if(size != len)
    {
        printf("fail to write : %s\n", fname);
        ret = -1;
    }

    if(fd)
    {
        close(fd);
    }
    return ret;
}

static int _smoke_bomb_cmd(unsigned int cmd, unsigned long sva, unsigned long eva)
{
	int r;
	struct smoke_bomb_cmd sb_cmd = {
		.cmd = cmd,
		.arg = {
			.sva = sva,
			.eva = eva,
		},
	};

	r = _smoke_bomb_write_to_lkm(SMOKE_BOMB_PROC_FULL_NAME, (char*)&sb_cmd, sizeof(sb_cmd));
	if (r)
		return r;

	return 0;
}

static void _smoke_bomb_ensure_page_map(unsigned long sva, unsigned long eva)
{
	/* [ToDo] support multiple pages */
	unsigned int val;
	unsigned int *ptr = (unsigned int *)sva;
	
	asm volatile ("ldr %0, [%1]\n": "=r" (val): "r" (ptr));
	asm volatile ("isb\n");

#ifdef _SMOKE_BOMB_ARMV7
    asm volatile ("dmb\n");	
#else
	asm volatile ("dmb ish\n");	
#endif
}

/* set FIFO scheduler to disable preemption, backup original attributes to parameters */
static int _smoke_bomb_set_sched_fifo(int *sched_policy, int *sched_prio)
{
	struct sched_param param;
	struct sched_param new_param;

	if (sched_getparam(0, &param) != 0) {
		printf("sched_getparam error : %d\n", errno);
		return -1;
	}
	*sched_policy = sched_getscheduler(0);
	*sched_prio = param.sched_priority;

	new_param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	if (sched_setscheduler(0, SCHED_FIFO, &new_param) != 0) {
		printf("sched_setscheduler error : %d\n", errno);
		return -1;
	}

	return 0;
}
static void _smoke_bomb_restore_sched(int sched_policy, int sched_prio)
{
	struct sched_param param;

	param.sched_priority = sched_prio;
	if (sched_setscheduler(0, sched_policy, &param) != 0) {
		printf("sched_setscheduler error : %d\n", errno);
		return;
	}
}

int smoke_bomb_init(unsigned long sva, unsigned long eva, int *sched_policy, int *sched_prio)
{
	_smoke_bomb_ensure_page_map(sva, eva);
	if (_smoke_bomb_set_sched_fifo(sched_policy, sched_prio))
		return -1;
	
	return _smoke_bomb_cmd(SMOKE_BOMB_CMD_INIT, sva, eva);
}

int smoke_bomb_exit(unsigned long sva, unsigned long eva, int sched_policy, int sched_prio)
{
	_smoke_bomb_ensure_page_map(sva, eva);
	_smoke_bomb_restore_sched(sched_policy, sched_prio);
	return _smoke_bomb_cmd(SMOKE_BOMB_CMD_EXIT, sva, eva);
}


/* api for debugging */
int smoke_bomb_init_pmu(void)
{
	return _smoke_bomb_cmd(SMOKE_BOMB_CMD_INIT_PMU, 0, 0);
}

int smoke_bomb_print_cpuid(void)
{
	return _smoke_bomb_cmd(SMOKE_BOMB_CMD_PRINT_CPUID, 0, 0);
}


