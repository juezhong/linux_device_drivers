// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 liyunfeng

// base
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

// some macro needed
#include <linux/device/driver.h> // device_driver, driver_register, driver_unregister
#include <linux/device.h> // dev_name, device
#include <linux/kdev_t.h> // MAJOR, MINOR

extern struct bus_type user_bus;

static int user_bus_driver_probe(struct device *dev)
{
	int ret = 0;
	printk("line:%4d  %s.\n", __LINE__, __func__);
	printk("line:%4d  dev dev->init_name: %s.\n", __LINE__, dev->init_name);
	printk("line:%4d  dev dev_name: %s.\n", __LINE__, dev_name(dev));
	printk("line:%4d  dev devt: %d, major: %d, minor: %d.\n", __LINE__,
	       dev->devt, MAJOR(dev->devt), MINOR(dev->devt));
	printk("line:%4d  dev address: %p.\n", __LINE__, dev);
	return ret;
};

static int user_bus_driver_remove(struct device *dev)
{
	int ret = 0;
	printk("line:%4d  %s.\n", __LINE__, __func__);
	return ret;
}

// Copy from ../device/bus_device.c
// static struct device user_bus_device = {
// 	.init_name = "user_bus_device",
// 	.bus = &user_bus,
// 	.devt = MKDEV(255, 0),
// 	.release = user_bus_device_release,
// };

static struct device_driver user_bus_driver = {
	.name = "user_bus_device", // 这里注意要和设备的名字一样，因为匹配是通过名称匹配
	.bus = &user_bus,
	.probe = user_bus_driver_probe,
	.remove = user_bus_driver_remove,
};

static int __init user_bus_driver_init(void)
{
	printk("Module init.\n");

	int ret = 0;
	ret = driver_register(&user_bus_driver);
	return ret;
}

static void __exit user_bus_driver_exit(void)
{
	printk("Module exit.\n");
	driver_unregister(&user_bus_driver);
}

module_init(user_bus_driver_init);
module_exit(user_bus_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("liyunfeng <1193230388@qq.com>");
MODULE_DESCRIPTION("This is a user define bus registe driver module example.");
MODULE_VERSION("0.1");
