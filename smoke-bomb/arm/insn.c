#include <linux/kernel.h>
#include <linux/init.h>

#include <asm/uaccess.h>
#include <asm/cacheflush.h>
#include <asm/pgtable.h>
#include <asm/tlbflush.h>
#include <asm/traps.h>
#include <asm/opcodes.h>

#include "insn.h"
#include "cache.h"

#define SB_INSN_DUMMY_1 0x07
#define SB_INSN_DUMMY_2 0x2
#define SB_INSN_DUMMY_3 0x1

/* INSN_OP range :  0x1 ~ 0x3. do not use 0x0 */
#define SB_INSN_OP_LDR_REG 0x1
#define SB_INSN_OP_LDR_IMM 0x2

typedef bool (*sb_insn_checker)(insn);
typedef sb_insn (*sb_insn_converter)(insn, insn);
typedef void (*sb_insn_dispatcher)(struct pt_regs *, sb_insn);

struct sb_insn_handler {
	sb_insn op;
	sb_insn_checker checker;
	sb_insn_converter converter;
	sb_insn_dispatcher dispatcher;
};

#define DEFINE_SB_INSN_HANDLER(sb_op, abbr) \
static struct sb_insn_handler handler_##abbr = { \
	.op = sb_op, \
	.checker = is_insn_##abbr, \
	.converter = convert_##abbr, \
	.dispatcher = dispatch_##abbr, \
}

/*
 *******************************************************
 * decoding function for smoke-bomb instruction - start
 *******************************************************
 */
static inline sb_insn sb_get_sb_op(sb_insn code)
{
	return ((code << 8) >> 30);
}
static inline sb_insn sb_get_rm(sb_insn code)
{
	return ((code << 28) >> 28);
}
static inline sb_insn sb_get_rn(sb_insn code)
{
	return ((code << 12) >> 28);
}
static inline sb_insn sb_get_rt(sb_insn code)
{
	return ((code << 16) >> 28);
}
/*
 *******************************************************
 * decoding function for smoke-bomb instruction - end
 *******************************************************
 */


/*
 *****************************************
 * INSN_HANDLER for LDR32-reg - start
 ******************************************
 */
static bool is_insn_ldr_reg(insn code)
{
	return (code & (0xFFF00000)) == (0xE7900000);	/* (code & mask) == val */
}
static sb_insn convert_ldr_reg(insn code, insn sb_op)
{
	sb_insn sb_code = 0;
	insn rn = 0, rt = 0, rm = 0;

	sb_code |= (SB_INSN_DUMMY_1 << 24);
	sb_code |= (SB_INSN_DUMMY_2 << 20);
	sb_code |= (SB_INSN_DUMMY_3 << 4);
	sb_code |= (sb_op << 22);

	rn = (code << 12) >> 28;
	rt = (code << 16) >> 28;
	rm = (code << 28) >> 28;

	sb_code |= (rn << 16);
	sb_code |= (rt << 12);
	sb_code |= (rm);

	return sb_code;
}
static void dispatch_ldr_reg(struct pt_regs *regs, sb_insn sb_code)
{
	sb_insn rn, rt, rm;
	unsigned int *ptr;
	unsigned int idx;

	rn = sb_get_rn(sb_code);
	rt = sb_get_rt(sb_code);
	rm = sb_get_rm(sb_code);

	/* LDR Rt, [Rn] ==> read data from Rn, write data to Rt. */
	/* 1. LDR */
	ptr = (unsigned int *)regs->uregs[rn];
	idx = (unsigned int)regs->uregs[rm];
	ptr += idx;
	regs->uregs[rt] = *ptr; /* perform original LDR instruction */

	/* 2. Flush */
	flush_dcache(ptr);
}
/*
 *****************************************
 * INSN_HANDLER for LDR32-reg - end
 ******************************************
 */


/*
 *****************************************
 * INSN_HANDLER for LDR32-imm - start
 ******************************************
 */
static bool is_insn_ldr_imm(insn code)
{
	return (code & (0xFFF88000)) == (0xE5900000);	/* (code & mask) == val && imm == 0 */
}
static sb_insn convert_ldr_imm(insn code, insn sb_op)
{
	sb_insn sb_code = 0;
	insn rn = 0, rt = 0;

	sb_code |= (SB_INSN_DUMMY_1 << 24);
	sb_code |= (SB_INSN_DUMMY_2 << 20);
	sb_code |= (SB_INSN_DUMMY_3 << 4);
	sb_code |= (sb_op << 22);

	rn = (code << 12) >> 28;
	rt = (code << 16) >> 28;

	sb_code |= (rn << 16);
	sb_code |= (rt << 12);

	return sb_code;
}
static void dispatch_ldr_imm(struct pt_regs *regs, sb_insn sb_code)
{
	sb_insn rn, rt;
	unsigned int *ptr;

	rn = sb_get_rn(sb_code);
	rt = sb_get_rt(sb_code);

	/* LDR Rt, [Rn] ==> read data from Rn, write data to Rt. */
	/* 1. LDR */
	ptr = (unsigned int *)regs->uregs[rn];
	regs->uregs[rt] = *ptr; /* perform original LDR instruction */

	/* 2. Flush */
	flush_dcache(ptr);
}
/*
 *****************************************
 * INSN_HANDLER for LDR32-imm - end
 ******************************************
 */


/*
 * [ converter_ldr_reg ]
 * We consier "ldr r3, [r3]" case only.
 * "ldr r3, [fp, #-20]" --> this case is ignored now.
 *
 * additionally, "ldr fp, [sp]" shoule be ignored, too.
 * To ignore the case, skip if Rn or Rt bigger than 0xA (r10)
 *
 * convert "ldr r3, [r3]" to "ldrflush r3, [r3]"
 */
DEFINE_SB_INSN_HANDLER(SB_INSN_OP_LDR_REG, ldr_reg);
DEFINE_SB_INSN_HANDLER(SB_INSN_OP_LDR_IMM, ldr_imm);

struct sb_insn_handler* handlers[] = {
	NULL,
	&handler_ldr_reg,
	&handler_ldr_imm,
};

/*
 * [ smoke-bomb instruction encoding rule ]
 *
 * ** not fixed.
 *
 * SB_INSN_DUMMY_* makes smoke-bomb instruction to be recognized as undef instruction.
 *
 * [31:24] - SB_INSN_DUMMY_1
 * [23:22] - smoke-bomb opcode (SB_INSN_OP_LDR32_REG, ...)
 * [21:20] - SB_INSN_DUMMY_2
 * [19:16] - Rn
 * [15:12] - Rt
 * [11:7] - imm5
 * [6:5] - type
 * [4] - SB_INSN_DUMMY_3
 * [3:0] - Rm
 *
 */
sb_insn convert_insn_to_sb_insn(insn code)
{
	unsigned i;
	bool r;

	for (i=1; i<ARRAY_SIZE(handlers); i++) {
		r = handlers[i]->checker(code);
		if (r)
			return handlers[i]->converter(code, handlers[i]->op);
	}

	return 0;
}

/* exception handler */
int smoke_bomb_ex_handler(struct pt_regs *regs, unsigned int instr)
{
	sb_insn sb_op;

	sb_op = sb_get_sb_op(instr);
	handlers[sb_op]->dispatcher(regs, instr);
	
	/* advance pc, and keep going!! */
    regs->ARM_pc += 4;
    return 0;
}


