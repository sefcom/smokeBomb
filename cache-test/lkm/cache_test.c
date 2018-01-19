/*
 * You may use any sim2.0 project under the terms of either of Samsung Proprietary License or
 * the GNU General Public License (GPL) Version 2.
 *
 * GPL v2.0 : Copyright(C)   2015 Samsung Electronics Co., Ltd.
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
#include <asm/uaccess.h>

struct test_vector {
	char cmd[64];
	void (*init)(void);
	void (*exit)(void);
};

/* intialize / finalize test functions */
void lru_test_init(void);
void lru_test_exit(void);
void line_fill_test_init(void);
void line_fill_test_exit(void);
void icache_test_init(void);
void icache_test_exit(void);


struct test_vector tests[] = {
	{
		.cmd = "lru",
		.init = lru_test_init,
		.exit = lru_test_exit,
	},
	{
		.cmd = "line_fill",
		.init = line_fill_test_init,
		.exit = line_fill_test_exit,
	},
	{
		.cmd = "icache_test",
		.init = icache_test_init,
		.exit = icache_test_exit,
	},
};

static int cache_test_open(struct inode *inode,struct file *filp)
{
	return 0;
}
static int cache_test_close(struct inode *inode,struct file *filp)
{
	return 0;
}
static ssize_t cache_test_read(struct file *file, char *buf, size_t count, loff_t *off)
{
	return 0;
}

/*
 * Command is..
 * sh>> echo -n "[test name]" > /proc/cache_test
 *    e.g) sh>> echo -n "lru" > /proc/cache_test
 */
static ssize_t cache_test_write(struct file *file, const char *buf, size_t count, loff_t *data)
{
	char cmd[64] = {0,};
	int ret;
	unsigned i;

	ret = copy_from_user(cmd, buf, count);
	if (ret) {
		pr_err("copy_from_user error\n");
		return ret;
	}

	for (i=0; i<ARRAY_SIZE(tests); i++) {	
		if (strcmp(cmd, tests[i].cmd) == 0) {
			tests[i].init();
			tests[i].exit();
			return count;
		}
	}

	pr_err("do not support [%s] test\n", cmd);
	return count;
}

static struct file_operations cache_test_fops =
{
	.owner = THIS_MODULE,
	.read = cache_test_read,
	.write = cache_test_write,
	.open = cache_test_open,
	.release = cache_test_close,
};

int __init cache_test_init(void)
{
	pr_info("cache_test_init\n");
	proc_create("cache_test", 0, NULL, &cache_test_fops);
	return 0;
}

void __exit cache_test_exit(void)
{
	pr_info("cache_test_exit\n");
	remove_proc_entry("cache_test", NULL);
}

module_init(cache_test_init);
module_exit(cache_test_exit);
MODULE_LICENSE("GPL");

