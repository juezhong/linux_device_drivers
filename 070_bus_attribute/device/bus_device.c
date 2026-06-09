// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 liyunfeng

// base
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

// some macro needed
#include <linux/device.h> // device, device_register, device_unregister
#include <linux/kdev_t.h> // MKDEV

extern struct bus_type user_bus;

static void user_bus_device_release(struct device *dev)
{
	printk("line:%4d  %s.\n", __LINE__, __func__);
}

static struct device user_bus_device = {
	.init_name = "user_bus_device",
	.bus = &user_bus,
	.devt = MKDEV(255, 0),
	.release = user_bus_device_release,
};

static int __init user_bus_device_init(void)
{
	printk("Module init.\n");

	int ret = 0;
	ret = device_register(&user_bus_device);
	return ret;
}

static void __exit user_bus_device_exit(void)
{
	printk("Module exit.\n");
	device_unregister(&user_bus_device);
}

module_init(user_bus_device_init);
module_exit(user_bus_device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("liyunfeng <1193230388@qq.com>");
MODULE_DESCRIPTION("This is a user define bus registe device module example.");
MODULE_VERSION("0.1");