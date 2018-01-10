#ifndef _SMOKE_BOMB_CACHE_H
#define _SMOKE_BOMB_CACHE_H

/* PMU related */
#define ARMV8_PMCR_E            (1 << 0) /* Enable all counters */
#define ARMV8_PMCR_P            (1 << 1) /* Reset all counters */
#define ARMV8_PMCR_C            (1 << 2) /* Cycle counter reset */
#define ARMV8_PMCNTENSET_EL0_EN (1 << 31) /* Performance Monitors Count Enable Set register */

#define ARMV8_PMUUSERNR_EN		(1 << 0)

static inline void init_pmu(void)
{
    uint32_t value = 0;
    asm volatile("MRS %0, PMCR_EL0" : "=r" (value));
    value |= ARMV8_PMCR_E;
    value |= ARMV8_PMCR_C;
    value |= ARMV8_PMCR_P;
    asm volatile("MSR PMCR_EL0, %0" : : "r" (value));
    asm volatile("MRS %0, PMCNTENSET_EL0" : "=r" (value));
    value |= ARMV8_PMCNTENSET_EL0_EN;
    asm volatile("MSR PMCNTENSET_EL0, %0" : : "r" (value));

    /* user enable */
    asm volatile("MRS %0, PMUSERENR_EL0" : "=r" (value));
    value |= ARMV8_PMUUSERNR_EN;
    asm volatile("MSR PMUSERENR_EL0, %0" :: "r" (value));
}

static inline void flush_dcache(void *addr)
{
	asm volatile ("DC CIVAC, %0" :: "r"(addr));
    asm volatile ("DSB ISH");
    asm volatile ("ISB");
}

#endif

