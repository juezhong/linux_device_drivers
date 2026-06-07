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
#include <linux/container_of.h> // container_of

// kobjet needed
#include <linux/kobject.h>
#include <linux/sysfs.h> // attribute, attribute_group, bin_attribute, file, sysfs_ops
#include <linux/types.h> // loff_t, ssize_t

// define user struct
typedef struct user_kobject {
	struct kobject _kobj;
	struct kobj_type _ktype;
	int value_1;
	int value_2;
	char bin_buf_1[1024];
} user_kobject_t;

static void _user_dynamic_kobj_release(struct kobject *kobj)
{
	printk("line:%4d  [%s] kobj name:%s (%p): %s\n", __LINE__,
	       module_name(THIS_MODULE), kobj->name, kobj, __func__);
	/*
	 * user_kobject 是 static 全局变量，内存由模块加载器管理，
	 * 模块卸载时自动回收，无需也不能 kfree。
	 * 如果后续改为 kzalloc 动态分配，这里再打开 kfree。
	 */
	// user_kobject_t *user_kobject =
	// 	container_of(kobj, user_kobject_t, _kobj);
	// kfree(user_kobject);
}

static ssize_t user_sysfs_ops_show(struct kobject *kobj, struct attribute *attr,
				   char *buf)
{
	int count = 0;
	user_kobject_t *user_kobject =
		container_of(kobj, struct user_kobject, _kobj);
	if (strcmp("user_attr_1_value1", attr->name) == 0) {
		count = sprintf(buf, "user_attr_1_value1: %d\n",
				user_kobject->value_1);
	}

	if (strcmp("user_attr_1_value2", attr->name) == 0) {
		count = sprintf(buf, "user_attr_1_value2: %d\n",
				user_kobject->value_2);
	}

	if (strcmp("user_attr_2_value1", attr->name) == 0) {
		count = sprintf(buf, "user_attr_2_value1: %d\n",
				user_kobject->value_1);
	}

	if (strcmp("user_attr_2_value2", attr->name) == 0) {
		count = sprintf(buf, "user_attr_2_value2: %d\n",
				user_kobject->value_2);
	}

	return count;
}
static ssize_t user_sysfs_ops_store(struct kobject *kobj,
				    struct attribute *attr, const char *buf,
				    size_t size)
{
	int ret;
	int value;
	user_kobject_t *user_kobject =
		container_of(kobj, struct user_kobject, _kobj);

	ret = kstrtoint(buf, 10, &value);
	if (ret < 0)
		return ret;

	if (strcmp("user_attr_1_value1", attr->name) == 0)
		user_kobject->value_1 = value;
	else if (strcmp("user_attr_1_value2", attr->name) == 0)
		user_kobject->value_2 = value;
	else if (strcmp("user_attr_2_value1", attr->name) == 0)
		user_kobject->value_1 = value;
	else if (strcmp("user_attr_2_value2", attr->name) == 0)
		user_kobject->value_2 = value;
	else
		return -EINVAL; /* unknown attribute */

	return size; /* 告知内核已消费全部数据，避免重试死循环 */
}

static struct sysfs_ops user_sysfs_ops = {
	.show = user_sysfs_ops_show,
	.store = user_sysfs_ops_store,
};

// define group 1
static struct attribute user_attr_1_value1 = {
	.name = "user_attr_1_value1",
	.mode = 0644,
};

static struct attribute user_attr_1_value2 = {
	.name = "user_attr_1_value2",
	.mode = 0644,
};

// 内核提供了将 kobject 和 attribute 结合在一起的结构体
// 这是对应不同的 kobj 的属性操作，定义了 foo_show, foo_store 专门来操作这一组
// 同时注意上面的 sysfs_ops 要回调这两个对应的实现的操作，也就是通过参数反向找到 user_kobj_attr 这个结构体
// 然后再调用对应的处理
// static struct kobj_attribute user_kobj_attr = {
// 	.attr = {
// 		.name = "user_kobj_attr_name",
// 		.mode = 0644,
// 	},
// 	.show = foo_show,
// 	.store = foo_store,
// };
//
// static struct kobj_attribute user_kobj_attr = __ATTR("user_kobj_attr_name", 0644, foo_show, foo_store);

static ssize_t user_bin_attribute_read(struct file *filp, struct kobject *kobj,
				       struct bin_attribute *bin_attr,
				       char *buf, loff_t offset, size_t size)
{
	user_kobject_t *user_kobject =
		container_of(kobj, user_kobject_t, _kobj);
	size_t buffer_size = sizeof(user_kobject->bin_buf_1);

	/* offset 是有符号类型，先防止负数 */
	if (offset < 0)
		return -EINVAL;

	/* 到达文件末尾 */
	if ((size_t)offset >= buffer_size)
		return 0;

	/* 防止越界读取 */
	if (size > buffer_size - (size_t)offset)
		size = buffer_size - (size_t)offset;

	/*
	 * buf 是 sysfs 提供的内核缓冲区，
	 * 这里直接使用 memcpy，不使用 copy_to_user。
	 */
	memcpy(buf, user_kobject->bin_buf_1 + offset, size);

	return size;
}
static ssize_t user_bin_attribute_write(struct file *filp, struct kobject *kobj,
					struct bin_attribute *bin_attr,
					char *buf, loff_t offset, size_t size)
{
	user_kobject_t *user_kobject =
		container_of(kobj, user_kobject_t, _kobj);
	size_t buffer_size = sizeof(user_kobject->bin_buf_1);

	if (offset < 0)
		return -EINVAL;

	/* 写入起点已经超出缓冲区 */
	if ((size_t)offset >= buffer_size)
		return -ENOSPC;

	/* 超过末尾时截断写入 */
	if (size > buffer_size - (size_t)offset)
		size = buffer_size - (size_t)offset;

	/*
	 * buf 已经是内核缓冲区，
	 * 直接 memcpy，不使用 copy_from_user。
	 */
	memcpy(user_kobject->bin_buf_1 + offset, buf, size);

	return size;
}

static struct bin_attribute user_bin_attribute_1 = {
	.attr = {
		.name = "user_bin_attr_1",
		.mode = 0644,
	},
	.read = user_bin_attribute_read,
	.write = user_bin_attribute_write,
	.size = 1024,
};

static struct attribute *user_attrs_1[] = {
	&user_attr_1_value1,
	&user_attr_1_value2,
	NULL,
};

static struct bin_attribute *user_bin_attrs_1[] = {
	&user_bin_attribute_1,
	NULL,
};

static struct attribute_group user_ktype_default_group_1 = {
	.attrs = user_attrs_1,
	.bin_attrs = user_bin_attrs_1,
	.name = "user_ktype_default_group_1",
};

// define group 2
static struct attribute user__attr_2_value1 = {
	.name = "user_attr_2_value1",
	.mode = 0444,
};

static struct attribute user__attr_2_value2 = {
	.name = "user_attr_2_value2",
	.mode = 0444,
};

// re-use bin-attr-1
static struct bin_attribute user_bin_attribute_2 = {
	.attr = {
		.name = "user_bin_attr_2",
		.mode = 0444,
	},
	.read = user_bin_attribute_read,
	.write = user_bin_attribute_write,
	.size = 1024,
};

static struct attribute *user_attrs_2[] = {
	&user__attr_2_value1,
	&user__attr_2_value2,
	NULL,
};

static struct bin_attribute *user_bin_attrs_2[] = {
	&user_bin_attribute_2,
	NULL,
};

static struct attribute_group user_ktype_default_group_2 = {
	.attrs = user_attrs_2,
	.bin_attrs = user_bin_attrs_2,
	// .name = "user_ktype_default_group_2",
};

static const struct attribute_group *user_ktype_default_groups[] = {
	&user_ktype_default_group_1,
	&user_ktype_default_group_2,
	NULL,
};

// define user struct
static user_kobject_t user_kobject = {
	._ktype = {
		.release = _user_dynamic_kobj_release,
		.default_groups = user_ktype_default_groups,
		.sysfs_ops = &user_sysfs_ops,
	},
	.value_1 = 1,
	.value_2 = 2,
	.bin_buf_1 = { '\0' },
};

static int __init kobj_kset_init(void)
{
	printk("Module init.\n");

	int ret = 0;

	// When parent is NULL, the kobject is created directly under /sys.
	// If the parent variable is NULL, the kobject is created under the /sys directory.
	ret = kobject_init_and_add(&user_kobject._kobj, &user_kobject._ktype,
				   NULL, "user_kobject_t_name");
	if (ret) {
		return -EINVAL;
	}
	printk("line:%4d  [%s]: kobj created.\n", __LINE__,
	       module_name(&__this_module));
	// printk("line:%4d  [%s]: kobj created.\n", __LINE__, module_name(THIS_MODULE));

	return ret;
}

static void __exit kobj_kset_exit(void)
{
	printk("Module exit.\n");

	// 实际是将引用计数器减一
	kobject_put(&user_kobject._kobj);
	printk("line:%4d  [%s]: kobj released.\n", __LINE__,
	       module_name(THIS_MODULE));
}

module_init(kobj_kset_init);
module_exit(kobj_kset_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("liyunfeng <1193230388@qq.com>");
MODULE_DESCRIPTION("This is a ktype&attribute module example.");
MODULE_VERSION("0.1");
