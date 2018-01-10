/*
 * You may use any sim2.0 project under the terms of either of Samsung Proprietary License or
 * the GNU General Public License (GPL) Version 2.
 *
 * GPL v2.0 : Copyright(C)   2017 Samsung Electronics Co., Ltd.
 *  http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Samsung Proprietary License : Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 * PROPRIETARY/CONFIDENTIAL
 * This software is the confidential and proprietary information of
 * Samsung Electronics CO.LTD.,. ("Confidential Information").
 * You shall not disclose such Confidential Information and shall
 * use it only in accordance with the terms of the license agreement
 * you entered into with Samsung Electronics CO.LTD.,.
 *
 * SAMSUNG make no representations or warranties about the suitability of the software,
 * either express or implied, including but not limited to the implied warranties of merchantability,
 * fitness for a particular purpose, or non-infringement.
 * SAMSUNG shall not be liable for any damages suffered by licensee as a result of using,
 * modifying or distributing this software or its derivatives.
 *
*/

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/path.h>
#include <linux/mount.h>
#include <linux/sched.h>
#include <asm/uaccess.h>

#include "../header.h"

/* intialize / finalize test functions */
static int smoke_bomb_cmd_init(struct smoke_bomb_cmd_arg *arg)
{
	int r;

	sb_pr_info("smoke_bomb_cmd_init start\n");

	/* 1. patch */
	r = patch_user_memory(arg->sva, arg->eva);
	if (r) {
		sb_pr_err("patch_user_memory error : %d\n", r);
		return r;
	}

	sb_pr_info("smoke_bomb_cmd_init end\n");
	return 0;
}

static int smoke_bomb_cmd_exit(struct smoke_bomb_cmd_arg *arg)
{
	sb_pr_info("smoke_bomb_cmd_exit start\n");
	return 0;
}

static int smoke_bomb_cmd_init_pmu(struct smoke_bomb_cmd_arg *arg)
{
	sb_pr_info("smoke_bomb_cmd_init_pmu start\n");
	init_pmu();
	return 0;
}

static int smoke_bomb_print_cpuid(struct smoke_bomb_cmd_arg *arg)
{
	sb_pr_info("cpuid : %d, pid : %d\n", smp_processor_id(), current->pid);
	return 0;
}

static struct smoke_bomb_cmd_vector cmds[] = {
	{
		.cmd = SMOKE_BOMB_CMD_INIT,
		.func = smoke_bomb_cmd_init,
	},
	{
		.cmd = SMOKE_BOMB_CMD_EXIT,
		.func = smoke_bomb_cmd_exit,
	},
	{
		.cmd = SMOKE_BOMB_CMD_INIT_PMU,
		.func = smoke_bomb_cmd_init_pmu,
	},
	{
		.cmd = SMOKE_BOMB_CMD_PRINT_CPUID,
		.func = smoke_bomb_print_cpuid,
	},
};

static int smoke_bomb_open(struct inode *inode,struct file *filp)
{
	return 0;
}
static int smoke_bomb_close(struct inode *inode,struct file *filp)
{
	return 0;
}
static ssize_t smoke_bomb_read(struct file *file, char *buf, size_t count, loff_t *off)
{
	return 0;
}

static ssize_t smoke_bomb_write(struct file *file, const char *buf, size_t count, loff_t *data)
{
	struct smoke_bomb_cmd cmd;
	int ret;

	ret = copy_from_user(&cmd, buf, count);
	if (ret) {
		sb_pr_err("copy_from_user error\n");
		return count;
	}

	if (cmd.cmd >= ARRAY_SIZE(cmds)) {
		sb_pr_err("cmd [%d] is not supported\n", cmd.cmd);
		return count;
	}

	/* print cmd info */
	sb_pr_info("cmd : %d\n", cmd.cmd);
	sb_pr_info("sva : %lx\n", cmd.arg.sva);
	sb_pr_info("eva : %lx\n", cmd.arg.eva);

	/* do cmd function */
	cmds[cmd.cmd].func(&cmd.arg);
	return count;
}

static struct file_operations smoke_bomb_fops =
{
	.owner = THIS_MODULE,
	.read = smoke_bomb_read,
	.write = smoke_bomb_write,
	.open = smoke_bomb_open,
	.release = smoke_bomb_close,
};

#include <asm/opcodes.h>
int __init smoke_bomb_init(void)
{
	int r;
	
	sb_pr_info("smoke_bomb_init\n");
	proc_create(SMOKE_BOMB_PROC_NAME, 0, NULL, &smoke_bomb_fops);
	
	r = fix_unresolve_function_ptrs();
	sb_pr_info("fix_unresolve_function_ptrs : %d\n", r);

	register_ex_handler();
	init_pmu();
	return 0;
}

void __exit smoke_bomb_exit(void)
{
	sb_pr_info("smoke_bomb_exit\n");
	remove_proc_entry(SMOKE_BOMB_PROC_NAME, NULL);
	unregister_ex_handler();
}

module_init(smoke_bomb_init);
module_exit(smoke_bomb_exit);
MODULE_LICENSE("GPL");

