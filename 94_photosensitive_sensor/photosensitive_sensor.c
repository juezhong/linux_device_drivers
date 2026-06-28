// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 liyunfeng

// base
#include <linux/kernel.h>
#include <linux/init.h> // THIS_MODULE, __exit, __init
#include <linux/module.h> // MODULE_AUTHOR, MODULE_DESCRIPTION, MODULE_LICENSE, MODULE_VERSION, module_exit, module_init

// some macro needed
#include <asm-generic/errno-base.h> // EFAULT, EINVAL, ENOMEM
#include <linux/slab.h> // kfree, kzalloc
#include <linux/gfp_types.h> // GFP_KERNEL
#include <linux/stddef.h> // NULL
#include <linux/types.h> // dev_t, loff_t, ssize_t
#include <linux/cdev.h> // cdev, cdev_add, cdev_del, cdev_init
#include <linux/fs.h> // alloc_chrdev_region, file, file_operations, inode, unregister_chrdev_region
#include <linux/compiler_types.h> // __iomem
#include <linux/device.h> // device, device_create, device_destroy
#include <linux/device/class.h> // class, class_create, class_destroy
#include <linux/err.h> // IS_ERR, PTR_ERR
#include <linux/gpio/consumer.h> // GPIOD_ASIS, devm_gpiod_get, gpio_desc, gpiod_direction_input, gpiod_get_value
#include <linux/kern_levels.h> // KERN_INFO
#include <linux/mod_devicetable.h> // of_device_id
#include <linux/platform_device.h> // platform_device, platform_driver, platform_driver_register, platform_driver_unregister, platform_set_drvdata
#include <linux/uaccess.h> // copy_to_user

struct photosensitive_sensor {
	void __iomem *base; // 没有需要转换的寄存器地址，占位
	int major; // 主设备号
	int minor; // 次设备号
	dev_t dev_t; // 设备号
	struct cdev cdev; // cdev结构体
	struct class *photosensitive_sensor_class; // 类
	struct device *photosensitive_sensor_dev; // 设备
	struct gpio_desc *gpiod;
	int value;
};
static struct photosensitive_sensor *photosensitive_sensor = NULL;

static int photosensitive_sensor_open(struct inode *inode, struct file *filp)
{
	filp->private_data = photosensitive_sensor;
	printk(KERN_INFO "photosensitive_sensor Character Device is opened\n");
	return 0;
}

static int photosensitive_sensor_release(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "photosensitive_sensor Character Device is closed\n");
	return 0;
}

static int photosensitive_sensor_get_value(struct photosensitive_sensor *sensor)
{
	// gpiod_get_value 返回 0/1 对应 GPIO 电平，失败时返回负的错误码
	return gpiod_get_value(sensor->gpiod);
}

static ssize_t photosensitive_sensor_read(struct file *filp, char *buff,
					  size_t len, loff_t *loff_t)
{
	struct photosensitive_sensor *sensor = filp->private_data;
	int value; // [0,1]
	int ret = 0;

	// 防止用户传入的 buff 长度小于 sizeof(int)，copy_to_user 会越界
	if (len < sizeof(value))
		return -EINVAL;

	// read the photosensitive_sensor value by gpio input.
	// Construct an auxiliary function
	value = photosensitive_sensor_get_value(sensor);
	if (value < 0)
		return value;
	// 另一种方式
	// photosensitive_sensor_get_value(sensor);
	// value = sensor->value;
	ret = copy_to_user(buff, &value, sizeof(value));
	// copy_to_user 返回 0 表示成功，>0 表示未完成的字节数，需转为 -EFAULT
	if (ret)
		return -EFAULT;

	// 更新文件偏移，否则用户重复 read 时无法判断是否已读完
	*loff_t += sizeof(value);
	return sizeof(value); // 返回实际读取的字节数
}

static struct file_operations photosensitive_sensor_fops = {
	.owner = THIS_MODULE,
	.open = photosensitive_sensor_open,
	.release = photosensitive_sensor_release,
	.read = photosensitive_sensor_read,
};

static int photosensitive_sensor_probe(struct platform_device *pdev)
{
	printk(KERN_INFO "line:%4d  %s.\n", __LINE__, __func__);
	int ret = 0;

	platform_set_drvdata(pdev, photosensitive_sensor);

	// register photosensitive_sensor character
	// 0. alloc device numebr
	ret = alloc_chrdev_region(&photosensitive_sensor->dev_t, 0, 1,
				  "photosensitive_sensor");
	if (ret) {
		printk(KERN_INFO "line:%4d  alloc_chrdev_region error.\n",
		       __LINE__);
		goto devt_error;
	}
	// 1. init cdev & add cdev
	cdev_init(&photosensitive_sensor->cdev, &photosensitive_sensor_fops);
	cdev_add(&photosensitive_sensor->cdev, photosensitive_sensor->dev_t, 1);
	// 2. alloc class
	photosensitive_sensor->photosensitive_sensor_class =
		class_create("sensors");
	if (IS_ERR(photosensitive_sensor->photosensitive_sensor_class)) {
		ret = PTR_ERR(
			photosensitive_sensor->photosensitive_sensor_class);
		goto class_error;
	}
	// 3. create device
	// parent = &pdev->dev, 将字符设备挂到平台设备下，建立 sysfs 层级关系
	photosensitive_sensor->photosensitive_sensor_dev = device_create(
		photosensitive_sensor->photosensitive_sensor_class, &pdev->dev,
		photosensitive_sensor->dev_t, photosensitive_sensor,
		"photosensitive_sensor");
	if (IS_ERR(photosensitive_sensor->photosensitive_sensor_dev)) {
		ret = PTR_ERR(photosensitive_sensor->photosensitive_sensor_dev);
		goto device_error;
	}

	// GPIO setting by GPIO descriptor
	// 0. get gpio desc
	photosensitive_sensor->gpiod =
		devm_gpiod_get(&pdev->dev, "photosensitive_sensor", GPIOD_ASIS);
	if (IS_ERR(photosensitive_sensor->gpiod)) {
		ret = PTR_ERR(photosensitive_sensor->gpiod);
		goto gpio_error;
	}
	printk(KERN_INFO "line:%4d  gpiod desc = %px.\n", __LINE__,
	       photosensitive_sensor->gpiod);
	// 1. set direction(input)
	ret = gpiod_direction_input(photosensitive_sensor->gpiod);
	if (ret)
		goto gpio_error;
	// 2. get value by read ops
	// see the read ops.

	return ret;

gpio_error:
	device_destroy(photosensitive_sensor->photosensitive_sensor_class,
		       photosensitive_sensor->dev_t);
device_error:
	class_destroy(photosensitive_sensor->photosensitive_sensor_class);
class_error:
	cdev_del(&photosensitive_sensor->cdev);
	unregister_chrdev_region(photosensitive_sensor->dev_t, 1);
devt_error:
	return ret;
}

static const struct of_device_id photosensitive_sensor_match_table[] = {
	{
		.compatible = "photosensitive_sensor",
	},
	{ /* sentinel */ },
};

static struct platform_driver photosensitive_sensor_platform_driver = {
    .driver = {
        .owner = THIS_MODULE,
        .name="photosensitive_sensor",
        .of_match_table = photosensitive_sensor_match_table,
    },
    .probe = photosensitive_sensor_probe,
};

/*
 * Module initialization and cleanup functions
 */
static int __init photosensitive_sensor_init(void)
{
	printk(KERN_INFO "line:%4d  %s.\n", __LINE__, __func__);
	int ret = 0;

	// allocate photosensitive_sensor structure
	photosensitive_sensor =
		kzalloc(sizeof(*photosensitive_sensor), GFP_KERNEL);
	if (!photosensitive_sensor)
		return -ENOMEM;
	printk(KERN_INFO
	       "line:%4d  allocate photosensitive_sensor structure.\n",
	       __LINE__);

	// 注意注册 platform 驱动的时候不仅仅是注册，同时也会进行扫描，也就意味着会立即匹配
	// 因为此时已经通过设备树加载了，所以要在注册之前准备好数据，否则 probe 的时候出现空指针异常
	// register platform driver(photosensitive_sensor)
	ret = platform_driver_register(&photosensitive_sensor_platform_driver);
	if (ret)
		goto driver_register_error;

	return ret;

driver_register_error:
	kfree(photosensitive_sensor);
	return ret;
}

static void __exit photosensitive_sensor_exit(void)
{
	printk(KERN_INFO "line:%4d  %s.\n", __LINE__, __func__);

	// cdev cleanup
	device_destroy(photosensitive_sensor->photosensitive_sensor_class,
		       photosensitive_sensor->dev_t);
	class_destroy(photosensitive_sensor->photosensitive_sensor_class);
	cdev_del(&photosensitive_sensor->cdev);
	unregister_chrdev_region(photosensitive_sensor->dev_t, 1);

	// free photosensitive_sensor structure
	kfree(photosensitive_sensor);

	// unregister character device
	platform_driver_unregister(&photosensitive_sensor_platform_driver);
}

module_init(photosensitive_sensor_init);
module_exit(photosensitive_sensor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("liyunfeng <1193230388@qq.com>");
MODULE_DESCRIPTION("attempt write a photosensitive sensor driver all by self.");
MODULE_VERSION("0.1");
