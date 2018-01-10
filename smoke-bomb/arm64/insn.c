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

#define SB_INSN_DUMMY_1 0x1 /* [31:26] ==> (SB_INSN_DUMMY_1 << 26) */
#define SB_INSN_DUMMY_2 0x7	/* [23:21] ==> (SB_INSN_DUMMY_2 << 21) */
#define SB_INSN_OP_LDR32_REG 0x0	/* [25:24] ==> (SB_INSN_OP_LDR32_REG << 24) */
#define SB_INSN_OP_LDR32_IMM 0x1

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
	return ((code << 6) >> 30);
}
static inline sb_insn sb_get_rm(sb_insn code)
{
	return ((code << 11) >> 27);
}
static inline sb_insn sb_get_rn(sb_insn code)
{
	return ((code << 22) >> 27);
}
static inline sb_insn sb_get_rt(sb_insn code)
{
	return ((code << 27) >> 27);
}
static inline sb_insn sb_get_option(sb_insn code)
{
	return ((code << 16) >> 29);
}
static inline sb_insn sb_get_s(sb_insn code)
{
	return ((code << 19) >> 31);
}
/*
 *******************************************************
 * decoding function for smoke-bomb instruction - end
 *******************************************************
 */


/*
 *****************************************
 * INSN_HANDLER for LDR32-register - start
 ******************************************
 */
static bool is_insn_ldr32_reg(insn code)
{
	/* (code & mask) == val */
	return ((code & (0xFFE00C00)) == (0xB8600800));
}
static sb_insn convert_ldr32_reg(insn code, insn sb_op)
{
	sb_insn sb_code = 0;
	insn rm = 0, option = 0, S = 0, rn = 0, rt = 0;

	/* 1. set dummy to sb_code */
	sb_code |= (SB_INSN_DUMMY_1 << 26);
	sb_code |= (SB_INSN_DUMMY_2 << 21);
	sb_code |= (sb_op << 24);

	/* 2. get info from code */
	rm = (code << 11) >> 27;
	rn = (code << 22) >> 27;
	rt = (code << 27) >> 27;
	option = (code << 16) >> 29;
	S = (code << 19) >> 31;
	
	/* 3. set info to sb_code */
	sb_code |= (rm << 16);
	sb_code |= (rn << 5);
	sb_code |= (rt);
	sb_code |= (option << 13);
	sb_code |= (S << 12);

	return sb_code;
}
static void dispatch_ldr32_reg(struct pt_regs *regs, sb_insn sb_code)
{
	sb_insn rm, rn, rt;
	unsigned int *ptr;
	unsigned int idx;

	rm = sb_get_rm(sb_code);
	rn = sb_get_rn(sb_code);
	rt = sb_get_rt(sb_code);
	
	/* 1. read addr from Rn */
	ptr = (unsigned int *)regs->regs[rn];
	idx = (unsigned int)regs->regs[rm];
	
	ptr += idx;
	regs->regs[rt] = *ptr; /* perform original LDR instruction */

	/* 2. Flush */
	flush_dcache(ptr);
}
/*
 *****************************************
 * INSN_HANDLER for LDR32-register - end
 ******************************************
 */

/*
 *****************************************
 * INSN_HANDLER for LDR32-imm - start
 ******************************************
 */
static bool is_insn_ldr32_imm(insn code)
{
	/* support (imm == 0) only */
	return ((code & (0xFFFFFC00)) == (0xB9400000));
}
static sb_insn convert_ldr32_imm(insn code, insn sb_op)
{
	sb_insn sb_code = 0;
	insn rn = 0, rt = 0;

	/* 1. set dummy to sb_code */
	sb_code |= (SB_INSN_DUMMY_1 << 26);
	sb_code |= (SB_INSN_DUMMY_2 << 21);
	sb_code |= (sb_op << 24);

	/* 2. get info from code */
	rn = (code << 22) >> 27;
	rt = (code << 27) >> 27;
	
	/* 3. set info to sb_code */
	sb_code |= (rn << 5);
	sb_code |= (rt);

	return sb_code;
}
static void dispatch_ldr32_imm(struct pt_regs *regs, sb_insn sb_code)
{
	sb_insn rn, rt;
	unsigned int *ptr;

	rn = sb_get_rn(sb_code);
	rt = sb_get_rt(sb_code);
	
	/* 1. read addr from Rn */
	ptr = (unsigned int *)regs->regs[rn];
	regs->regs[rt] = *ptr; /* perform original LDR instruction */

	/* 2. Flush */
	flush_dcache(ptr);
}
/*
 *****************************************
 * INSN_HANDLER for LDR32-imm - end
 ******************************************
 */


/*
 * [ handlers ]
 */
DEFINE_SB_INSN_HANDLER(SB_INSN_OP_LDR32_REG, ldr32_reg);
DEFINE_SB_INSN_HANDLER(SB_INSN_OP_LDR32_IMM, ldr32_imm);


struct sb_insn_handler* handlers[] = {
	&handler_ldr32_reg,
	&handler_ldr32_imm,
};

/*
 * [ smoke-bomb instruction encoding rule ]
 *
 * ** not fixed.
 *
 * SB_INSN_DUMMY_* makes smoke-bomb instruction to be recognized as undef instruction.
 *
 * [31:26] - SB_INSN_DUMMY_1
 * [25:24] - smoke-bomb opcode (SB_INSN_OP_LDR32_REG, ...)
 * [23:21] - SB_INSN_DUMMY_2
 * [20:16] - Rm
 * [15:13] - option
 * [12] - S
 * [9:5] - Rn
 * [4:0] - Rt
 *
 * e.g) ldr Wt, [Rn] ==> read data from Rn, write data to Wt. (W means word, 32bit register)
 */
sb_insn convert_insn_to_sb_insn(insn code)
{
	unsigned i;
	bool r;

	for (i=0; i<ARRAY_SIZE(handlers); i++) {
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
    regs->pc += 4;
    return 0;
}


