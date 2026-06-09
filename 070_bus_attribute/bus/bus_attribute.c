// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 liyunfeng

// base
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

// some macro needed
#include <asm-generic/errno-base.h> // EINVAL
#include <linux/device.h> //dev_name, device
#include <linux/device/driver.h> //device_driver
#include <linux/export.h> //EXPORT_SYMBOL_GPL

// bus needed
#include <linux/device/bus.h> // bus_register, bus_type, bus_unregister

static int user_bus_match(struct device *dev, const struct device_driver *drv)
{
	return !strcmp(dev_name(dev), drv->name);
}

static int user_bus_probe(struct device *dev)
{
	int ret = 0;
	struct device_driver *drv = dev->driver;
	if (drv->probe)
		ret = drv->probe(dev); // 手动转发
	return ret;
}

struct bus_type user_bus = {
	.name = "user_bus",
	.match = user_bus_match,
	.probe = user_bus_probe,
};
EXPORT_SYMBOL_GPL(user_bus);

static ssize_t user_bus_attr_show(const struct bus_type *bus, char *buf)
{
	return sprintf(buf, "line:%4d  %s\n", __LINE__, __func__);
}
static ssize_t user_bus_attr_store(const struct bus_type *bus, const char *buf,
				   size_t count)
{
	printk("Not implement.\n");
	return count;
}

struct bus_attribute user_bus_attr = {
	.attr = {
		.name = "user_bus_attr_value_name",
		.mode = 0644,
	},
	.show = user_bus_attr_show,
	.store = user_bus_attr_store,
};

static int __init kobj_kset_init(void)
{
	printk("Module init.\n");

	int ret = 0;
	ret = bus_register(&user_bus);
	if (ret)
		goto bus_register_err;
	ret = bus_create_file(&user_bus, &user_bus_attr);
	if (ret)
		goto bus_create_file_err;
	return ret;

bus_create_file_err:
	bus_unregister(&user_bus);
bus_register_err:
	return ret;
}

static void __exit kobj_kset_exit(void)
{
	printk("Module exit.\n");
	bus_remove_file(&user_bus, &user_bus_attr);
	bus_unregister(&user_bus);
}

module_init(kobj_kset_init);
module_exit(kobj_kset_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("liyunfeng <1193230388@qq.com>");
MODULE_DESCRIPTION("This is a bus&bus_attribute module example.");
MODULE_VERSION("0.1");