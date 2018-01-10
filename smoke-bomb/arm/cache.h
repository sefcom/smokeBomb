#ifndef _SMOKE_BOMB_CACHE_H
#define _SMOKE_BOMB_CACHE_H

static inline void init_pmu(void)
{
    /* Enable user-mode access to counters. */
    asm volatile ("mcr p15, 0, %0, C9, C14, 0\n\t" :: "r" (1));

    /* Program PMU and enable all counters */
    asm volatile ("mcr p15, 0, %0, c9, c12, 0" :: "r"((1 | 16)));
    asm volatile ("mcr p15, 0, %0, c9, c12, 1" :: "r"(0x8000000f));
}

static inline void flush_dcache(void *addr)
{
	unsigned long addr_long;

	addr_long = (unsigned long)addr;
	asm volatile ("MCR p15, 0, %0, c7, c14, 1\n" :: "r" (addr_long));
    asm volatile ("isb\n");
    asm volatile ("dsb\n");
}

static inline void flush_icache(void *addr)
{
	unsigned long addr_long;

	addr_long = (unsigned long)addr;
	asm volatile ("MCR p15, 0, %0, c7, c5, 1\n" :: "r" (addr_long));
    asm volatile ("isb\n");
    asm volatile ("dsb\n");
}

static inline void flush_dtlb(void *addr)
{
	unsigned long addr_long;

	addr_long = (unsigned long)addr;
	asm volatile ("MCR p15, 0, %0, c8, c6, 1\n" :: "r" (addr_long));
    asm volatile ("dsb\n");
    asm volatile ("isb\n");
}

static inline void flush_itlb(void *addr)
{
	unsigned long addr_long;

	addr_long = (unsigned long)addr;
	asm volatile ("MCR p15, 0, %0, c8, c5, 1\n" :: "r" (addr_long));
    asm volatile ("dsb\n");
    asm volatile ("isb\n");
}

static inline void flush_dtlb_all(void)
{
	unsigned long dummy = 0;
	
	asm volatile ("MCR p15, 0, %0, c8, c6, 0\n" :: "r" (dummy));
    asm volatile ("dsb\n");
    asm volatile ("isb\n");
}

static inline void flush_itlb_all(void)
{
	unsigned long dummy = 0;
	
	asm volatile ("MCR p15, 0, %0, c8, c5, 0\n" :: "r" (dummy));
    asm volatile ("dsb\n");
    asm volatile ("isb\n");
}



#endif

