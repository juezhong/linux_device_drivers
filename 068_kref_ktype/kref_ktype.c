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

static void _user_dynamic_kobj_release(struct kobject *kobj)
{
	printk("line:%4d  [%s] kobj name:%s (%p): %s\n", __LINE__,
	       module_name(THIS_MODULE), kobj->name, kobj, __func__);
	kfree(kobj);
}

struct kobject *kobj;
struct kobject *sub_kobj_1;
struct kobject *sub_kobj_2;
struct kobj_type _user_ktype = {
	.release = _user_dynamic_kobj_release,
};

struct kset *_user_kset;
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
	printk("line:%4d  [%s]: kobj created.\n", __LINE__,
	       module_name(&__this_module));
	// printk("line:%4d  [%s]: kobj created.\n", __LINE__, module_name(THIS_MODULE));
	printk("line:%4d  kobj: %s(%p)'s kref count is %d.\n", __LINE__,
	       kobj->name, kobj, kobj->kref.refcount.refs.counter);

	sub_kobj_1 = kobject_create_and_add("sub_user_kobj_kset_name", kobj);
	if (!sub_kobj_1) {
		return -EINVAL;
	}
	printk("line:%4d  [%s]: sub_kobj_1 created.\n", __LINE__,
	       module_name(THIS_MODULE));
	printk("line:%4d  kobj: %s(%p)'s kref count is %d.\n", __LINE__,
	       kobj->name, kobj, kobj->kref.refcount.refs.counter);
	printk("line:%4d  kobj: %s(%p)'s kref count is %d.\n", __LINE__,
	       sub_kobj_1->name, sub_kobj_1,
	       sub_kobj_1->kref.refcount.refs.counter);

	sub_kobj_2 = kzalloc(sizeof(struct kobject), GFP_KERNEL);
	if (!sub_kobj_2) {
		return -EINVAL;
	}
	ret = kobject_init_and_add(sub_kobj_2, &_user_ktype, sub_kobj_1,
				   "sub_kobj_2_name_by_init&add");
	// ret = kobject_init_and_add(sub_kobj_2, &ktype, sub_kobj_1, "%s", "sub_kobj_2_name_by_init&add");
	printk("line:%4d  [%s]: sub_kobj_2 created.\n", __LINE__,
	       module_name(THIS_MODULE));
	printk("line:%4d  kobj: %s(%p)'s kref count is %d.\n", __LINE__,
	       kobj->name, kobj, kobj->kref.refcount.refs.counter);
	printk("line:%4d  kobj: %s(%p)'s kref count is %d.\n", __LINE__,
	       sub_kobj_1->name, sub_kobj_1,
	       sub_kobj_1->kref.refcount.refs.counter);
	printk("line:%4d  kobj: %s(%p)'s kref count is %d.\n", __LINE__,
	       sub_kobj_2->name, sub_kobj_2,
	       sub_kobj_2->kref.refcount.refs.counter);
	/**
	# tree /sys/user_kobj_name/
	/sys/user_kobj_name/
	`-- sub_user_kobj_kset_name
	`-- sub_kobj_2_name_by_init&add

	3 directories, 0 files
	 */

	// kset
	_user_kset = kset_create_and_add("user_kset_name", NULL, NULL);
	printk("line:%4d  [%s]: _user_kset created.\n", __LINE__,
	       module_name(THIS_MODULE));
	printk("line:%4d  kobj: %s(%p)'s kref count is %d.\n", __LINE__,
	       _user_kset->kobj.name, _user_kset,
	       _user_kset->kobj.kref.refcount.refs.counter);
	kset_sub_kobj_1 = kzalloc(sizeof(struct kobject), GFP_KERNEL);
	kset_sub_kobj_2 = kzalloc(sizeof(struct kobject), GFP_KERNEL);
	if (!kset_sub_kobj_1 || !kset_sub_kobj_2) {
		return -EINVAL;
	}
	kset_sub_kobj_1->kset = _user_kset;
	kset_sub_kobj_2->kset = _user_kset;

	ret = kobject_init_and_add(kset_sub_kobj_1, &_user_ktype, NULL,
				   "kset_sub_kobj_1_name_by_init&add");
	ret = kobject_init_and_add(kset_sub_kobj_2, &_user_ktype, NULL,
				   "kset_sub_kobj_2_name_by_init&add");
	printk("line:%4d  [%s]: kset_sub_kobj_1 created.\n", __LINE__,
	       module_name(THIS_MODULE));
	printk("line:%4d  [%s]: kset_sub_kobj_2 created.\n", __LINE__,
	       module_name(THIS_MODULE));
	printk("line:%4d  kobj: %s(%p)'s kref count is %d.\n", __LINE__,
	       _user_kset->kobj.name, _user_kset,
	       _user_kset->kobj.kref.refcount.refs.counter);
	printk("line:%4d  kobj: %s(%p)'s kref count is %d.\n", __LINE__,
	       kset_sub_kobj_1->name, kset_sub_kobj_1,
	       kset_sub_kobj_1->kref.refcount.refs.counter);
	printk("line:%4d  kobj: %s(%p)'s kref count is %d.\n", __LINE__,
	       kset_sub_kobj_2->name, kset_sub_kobj_2,
	       kset_sub_kobj_2->kref.refcount.refs.counter);
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
	printk("line:%4d  [%s]: kobj released.\n", __LINE__,
	       module_name(THIS_MODULE));
	printk("line:%4d  kobj: %s(%p)'s kref count is %d.\n", __LINE__,
	       kobj->name, kobj, kobj->kref.refcount.refs.counter);
	printk("line:%4d  kobj: %s(%p)'s kref count is %d.\n", __LINE__,
	       sub_kobj_1->name, sub_kobj_1,
	       sub_kobj_1->kref.refcount.refs.counter);
	printk("line:%4d  kobj: %s(%p)'s kref count is %d.\n", __LINE__,
	       sub_kobj_2->name, sub_kobj_2,
	       sub_kobj_2->kref.refcount.refs.counter);

	kobject_put(sub_kobj_1);
	printk("line:%4d  [%s]: sub_kobj_1 released.\n", __LINE__,
	       module_name(THIS_MODULE));
	printk("line:%4d  kobj: %s(%p)'s kref count is %d.\n", __LINE__,
	       kobj->name, kobj, kobj->kref.refcount.refs.counter);
	printk("line:%4d  kobj: %s(%p)'s kref count is %d.\n", __LINE__,
	       sub_kobj_1->name, sub_kobj_1,
	       sub_kobj_1->kref.refcount.refs.counter);
	printk("line:%4d  kobj: %s(%p)'s kref count is %d.\n", __LINE__,
	       sub_kobj_2->name, sub_kobj_2,
	       sub_kobj_2->kref.refcount.refs.counter);

	kobject_put(sub_kobj_2);
	printk("line:%4d  [%s]: sub_kobj_1 released.\n", __LINE__,
	       module_name(THIS_MODULE));
	printk("line:%4d  kobj: %s(%p)'s kref count is %d.\n", __LINE__,
	       kobj->name, kobj, kobj->kref.refcount.refs.counter);
	printk("line:%4d  kobj: %s(%p)'s kref count is %d.\n", __LINE__,
	       sub_kobj_1->name, sub_kobj_1,
	       sub_kobj_1->kref.refcount.refs.counter);
	printk("line:%4d  kobj: %s(%p)'s kref count is %d.\n", __LINE__,
	       sub_kobj_2->name, sub_kobj_2,
	       sub_kobj_2->kref.refcount.refs.counter);

	kobject_put(kset_sub_kobj_1);
	printk("line:%4d  [%s]: kset_sub_kobj_1 released.\n", __LINE__,
	       module_name(THIS_MODULE));
	printk("line:%4d  kobj: %s(%p)'s kref count is %d.\n", __LINE__,
	       _user_kset->kobj.name, _user_kset,
	       _user_kset->kobj.kref.refcount.refs.counter);
	printk("line:%4d  kobj: %s(%p)'s kref count is %d.\n", __LINE__,
	       kset_sub_kobj_2->name, kset_sub_kobj_2,
	       kset_sub_kobj_2->kref.refcount.refs.counter);

	kobject_put(kset_sub_kobj_2);
	printk("line:%4d  [%s]: kset_sub_kobj_2 released.\n", __LINE__,
	       module_name(THIS_MODULE));
	printk("line:%4d  kobj: %s(%p)'s kref count is %d.\n", __LINE__,
	       _user_kset->kobj.name, _user_kset,
	       _user_kset->kobj.kref.refcount.refs.counter);

	kset_unregister(_user_kset);
	printk("line:%4d  [%s]: _user_kset released.\n", __LINE__,
	       module_name(THIS_MODULE));
	printk("line:%4d  kobj: %s(%p)'s kref count is %d.\n", __LINE__,
	       _user_kset->kobj.name, _user_kset,
	       _user_kset->kobj.kref.refcount.refs.counter);
}

module_init(kobj_kset_init);
module_exit(kobj_kset_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("liyunfeng <1193230388@qq.com>");
MODULE_DESCRIPTION("This is a kref&ktype module example.");
MODULE_VERSION("0.1");