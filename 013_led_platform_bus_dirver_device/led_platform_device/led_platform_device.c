#include "linux/ioport.h"
#include "linux/platform_device.h"
#include <linux/module.h>

/**
设备模块是 platform 的设备信息，即 platform_device 的结构体
这种方式是没有使用设备树的方式，使用自定义的设备信息去和对应的 platform 驱动进行匹配
通过 platform_driver 结构体中的 name 和 platform_device 结构体中的 name 进行匹配

platform_device 的代码主要是描述一些硬件信息，比如设备名称，设备资源等
设备资源包括寄存器地址，中断号等，有不同的类型
 */
#define CCM_CCGR1_BASE_ADDR         0x020C406C
#define SW_MUX_GPIO1_IO03_BASE_ADDR 0x020E0068
#define SW_PAD_GPIO1_IO03_BASE_ADDR 0x020E02F4
#define GPIO1_DR_BASE_ADDR          0x0209C000
#define GPIO1_GDIR_BASE_ADDR        0x0209C004
#define REGISTER_SIZE               0X4

// static int led_char_release (struct inode *inode, struct file *filp) {
static void led_char_platform_release(struct device *dev) {
    printk("LED Character Platform Device is released.\n");
}

static struct resource led_resource[] = {
    {
        .start = CCM_CCGR1_BASE_ADDR,
        .end = CCM_CCGR1_BASE_ADDR + REGISTER_SIZE - 1,
        .flags = IORESOURCE_MEM,
    },
    {
        .start = SW_MUX_GPIO1_IO03_BASE_ADDR,
        .end = SW_MUX_GPIO1_IO03_BASE_ADDR + REGISTER_SIZE - 1,
        .flags = IORESOURCE_MEM,
    },
    {
        .start = SW_PAD_GPIO1_IO03_BASE_ADDR,
        .end = SW_PAD_GPIO1_IO03_BASE_ADDR + REGISTER_SIZE - 1,
        .flags = IORESOURCE_MEM,
    },
    {
        .start = GPIO1_DR_BASE_ADDR,
        .end = GPIO1_DR_BASE_ADDR + REGISTER_SIZE - 1,
        .flags = IORESOURCE_MEM,
    },
    {
        .start = GPIO1_GDIR_BASE_ADDR,
        .end = GPIO1_GDIR_BASE_ADDR + REGISTER_SIZE - 1,
        .flags = IORESOURCE_MEM,
    },
};

static struct platform_device led_platform_device = {
    // 注意 name 字段和在 platform driver 的文件里面设置的 设备 name 字段一致
    .name = "opendev-platform-led",
    .id = -1,
    .num_resources = ARRAY_SIZE(led_resource),
    .resource = led_resource,
    .dev = {
        .release = led_char_platform_release,
    }
};

/*
 * Module initialization and cleanup functions
 */
static int __init led_char_platform_init(void) {
    printk(KERN_INFO "LED Character Platform Device is initialized.\n");
    int ret = 0;
    ret = platform_device_register(&led_platform_device);
    return ret;
}

static void __exit led_char_platform_exit(void) {
    printk(KERN_INFO "LED Character Platform Device is exited.\n");
    platform_device_unregister(&led_platform_device);
}

module_init(led_char_platform_init);
module_exit(led_char_platform_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("liyunfeng");
MODULE_DESCRIPTION("LED Character Platform Device");
MODULE_VERSION("0.1");
