// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 liyunfeng

// base
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

// some macro needed
#include <asm-generic/errno-base.h> // EINVAL
#include <linux/slab.h> // kzalloc
#include <linux/gfp_types.h> // GFP_KERNEL
#include <linux/stddef.h> // NULL

// kobjet needed
#include <linux/kobject.h>

struct kobject *kobj;
struct kobject *sub_kobj_1;
struct kobject *sub_kobj_2;
struct kobj_type ktype;

struct kset *kset;
struct kobject *kset_sub_kobj_1;
struct kobject *kset_sub_kobj_2;

static int __init kobj_kset_init(void)
{
	printk("Module init.\n");

	int ret = 0;

	// When parent is NULL, the kobject is created directly under /sys.
	// If the parent variable is NULL, the kobject is created under the /sys directory.
	kobj = kobject_create_and_add("user_kobj_name", NULL);
	if (!kobj) {
		return -EINVAL;
	}

	sub_kobj_1 = kobject_create_and_add("sub_user_kobj_kset_name", kobj);
	if (!sub_kobj_1) {
		return -EINVAL;
	}

	sub_kobj_2 = kzalloc(sizeof(struct kobject), GFP_KERNEL);
	if (!sub_kobj_2) {
		return -EINVAL;
	}
	ret = kobject_init_and_add(sub_kobj_2, &ktype, sub_kobj_1,
				   "sub_kobj_2_name_by_init&add");
	// ret = kobject_init_and_add(sub_kobj_2, &ktype, sub_kobj_1, "%s", "sub_kobj_2_name_by_init&add");
	/**
	# tree /sys/user_kobj_name/
	/sys/user_kobj_name/
	`-- sub_user_kobj_kset_name
	`-- sub_kobj_2_name_by_init&add

	3 directories, 0 files
	 */

	// kset
	kset = kset_create_and_add("user_kset_name", NULL, NULL);
	kset_sub_kobj_1 = kzalloc(sizeof(struct kobject), GFP_KERNEL);
	kset_sub_kobj_2 = kzalloc(sizeof(struct kobject), GFP_KERNEL);
	if (!kset_sub_kobj_1 || !kset_sub_kobj_2) {
		return -EINVAL;
	}
	kset_sub_kobj_1->kset = kset;
	kset_sub_kobj_2->kset = kset;

	ret = kobject_init_and_add(kset_sub_kobj_1, &ktype, NULL,
				   "kset_sub_kobj_1_name_by_init&add");
	ret = kobject_init_and_add(kset_sub_kobj_2, &ktype, NULL,
				   "kset_sub_kobj_2_name_by_init&add");
	/**
	# tree /sys/user_kset_name/
	/sys/user_kset_name/
	|-- kset_sub_kobj_1_name_by_init&add
	`-- kset_sub_kobj_2_name_by_init&add

	3 directories, 0 files
	 */

	return ret;
}

static void __exit kobj_kset_exit(void)
{
	printk("Module exit.\n");

	// 实际是将引用计数器减一
	kobject_put(kobj);
	kobject_put(sub_kobj_1);
	kobject_put(sub_kobj_2);

	kobject_put(kset_sub_kobj_1);
	kobject_put(kset_sub_kobj_2);
	kset_unregister(kset);
}

module_init(kobj_kset_init);
module_exit(kobj_kset_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("liyunfeng <1193230388@qq.com>");
MODULE_DESCRIPTION("This is a kobject&kset module example.");
MODULE_VERSION("0.1");