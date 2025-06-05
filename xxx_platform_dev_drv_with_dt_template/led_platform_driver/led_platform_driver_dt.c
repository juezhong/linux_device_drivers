#include "linux/ioport.h"
#include "linux/minmax.h"
#include "linux/platform_device.h"
#include "linux/types.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/device/class.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>


struct led_char_platform {
    int major;                  // 主设备号
    int minor;                  // 次设备号
    dev_t dev_t;                // 设备号
    struct cdev cdev;           // cdev结构体
    struct class *cls;          // 类
    struct device *dev;         // 设备
    struct device_node *np;     // 设备树节点
    int led_gpio;               // LED对应的GPIO
};
static struct led_char_platform *led_char_platform = NULL;

enum LED_STATUS {
    LED_OFF = 0,
    LED_ON = 1,
    LED_SWITCH = 2,
};

static int led_char_platform_open (struct inode *inode, struct file *filp) {
    printk(KERN_INFO "LED Character Platform Device(Driver) is opened\n");
    // 保存这个 led_char_platform 结构体指针，以便其他函数使用
    // 如果不这么做，其他函数无法访问这个结构体，或者多个设备文件同时打开时，无法区分是哪个设备文件
    filp->private_data = led_char_platform;
    return 0;
}

static int led_char_platform_release (struct inode *inode, struct file *filp) {
    printk(KERN_INFO "LED Character Platform Device(Driver) is closed\n");
    return 0;
}

static ssize_t led_char_platform_read (struct file *filp, char *buff, size_t len, loff_t *loff_t) {
    char kbuf[32] = "abc";
    int ret;
    len = MIN(len, 32);
    // len = min(len, 32);
    ret = copy_to_user(buff, kbuf, len);
    if (ret < 0) {
        printk("Failed to copy data to user space.\n");
        return ret;
    }
    return 0;
}

static void led_ctrl_switch (enum LED_STATUS status) {
    switch (status) {
        case LED_ON:
            gpio_set_value(led_char_platform->led_gpio, 0);
            break;
        case LED_OFF:
            gpio_set_value(led_char_platform->led_gpio, 1);
            break;
        default:
            // switch led status, on->off, off->on
            gpio_set_value(led_char_platform->led_gpio, !gpio_get_value(led_char_platform->led_gpio));
            break;
    }
}

static ssize_t led_char_platform_write (struct file *filp, const char *buff, size_t len, loff_t *loff_t) {
    char kbuf[32] = {'\0'};
    int ret;
    len = MIN(len, 32);
    // len = min(len, 32);
    ret = copy_from_user(kbuf, buff, len);
    if (ret < 0) {
        printk("Failed to copy data from user space.\n");
        return ret;
    }
    printk("Write from user data: %s\n", kbuf);
    // led_ctrl_switch(strcmp(kbuf, "on") == 0 ? LED_ON : LED_OFF); // 调用 led_ctrl_switch 函数控制LED灯
    if (strcmp(kbuf, "on") == 0) {
        led_ctrl_switch(LED_ON);
    } else if (strcmp(kbuf, "off") == 0) {
        led_ctrl_switch(LED_OFF);
    } else {
        led_ctrl_switch(LED_SWITCH);
    }
    return ret;
}

static struct file_operations led_char_platform_fops = {
    .owner = THIS_MODULE,
    .open = led_char_platform_open,
    .release = led_char_platform_release,
    .read = led_char_platform_read,
    .write = led_char_platform_write,
};

static int led_char_platform_init_led (struct led_char_platform *led_char_platform) {
    int ret = 0;
    if (led_char_platform == NULL) {
        printk(KERN_ERR "led_char_platform is NULL\n");
        return -1;
    }

    // get gpio number from device tree
    led_char_platform->led_gpio = of_get_named_gpio(led_char_platform->np, "led-gpios", 0);
    if (led_char_platform->led_gpio < 0) {
        printk(KERN_ERR "Failed to get gpio number\n");
        return -1;
    }
    ret = gpio_direction_output(led_char_platform->led_gpio, 1);
    if (ret < 0) {
        printk(KERN_ERR "Failed to set gpio direction\n");
        return ret;
    }
    return 0;
}

static int led_char_platform_cdev_init (struct led_char_platform *led_char_platform) {
    int ret = 0;
    if (led_char_platform == NULL) {
        printk(KERN_ERR "led_char_platform is NULL\n");
        return -1;
    }
    // allocate major and minor
    ret = alloc_chrdev_region(&led_char_platform->dev_t, 0, 1, "led_chrdev");
    if (ret < 0) {
        printk(KERN_ERR "Failed to allocate major and minor\n");
        return ret;
    }
    led_char_platform->major = MAJOR(led_char_platform->dev_t);
    led_char_platform->minor = MINOR(led_char_platform->dev_t);
    printk(KERN_INFO "LED Character Platform Device(Driver) major: %d, minor: %d\n", led_char_platform->major, led_char_platform->minor);
    // init cdev
    cdev_init(&led_char_platform->cdev, &led_char_platform_fops);
    // add cdev to kernel
    ret = cdev_add(&led_char_platform->cdev, led_char_platform->dev_t, 1);
    if (ret < 0) {
        printk(KERN_ERR "Failed to add cdev\n");
        unregister_chrdev_region(led_char_platform->dev_t, 1);
        return ret;
    }
    // create class and device
    led_char_platform->cls = class_create( "led_class");
    if (IS_ERR(led_char_platform->cls)) {
        printk(KERN_ERR "Failed to create class\n");
        cdev_del(&led_char_platform->cdev);
        unregister_chrdev_region(led_char_platform->dev_t, 1);
        return PTR_ERR(led_char_platform->cls);
    }
    led_char_platform->dev = device_create(led_char_platform->cls, NULL, led_char_platform->dev_t, NULL, "led_chrdev");
    // led_char_platform->dev = device_create(led_char_platform->cls, NULL, led_char_platform->dev_t, NULL, "led_chrdev_%d", led_char_platform->minor);
    if (IS_ERR(led_char_platform->dev)) {
        printk(KERN_ERR "Failed to create device\n");
        class_destroy(led_char_platform->cls);
        cdev_del(&led_char_platform->cdev);
        unregister_chrdev_region(led_char_platform->dev_t, 1);
        return PTR_ERR(led_char_platform->dev);
    }

    return ret;
}

static void led_char_platform_cdev_exit (struct led_char_platform *led_char_platform) {
    if (led_char_platform == NULL) {
        printk(KERN_ERR "led_char_platform is NULL\n");
        return;
    }
    // destroy device and class
    device_destroy(led_char_platform->cls, led_char_platform->dev_t);
    class_destroy(led_char_platform->cls);
    // remove cdev from kernel and unregister major and minor
    cdev_del(&led_char_platform->cdev);
    unregister_chrdev_region(led_char_platform->dev_t, 1);
}

static int led_char_platform_probe(struct platform_device *pdev) {
    // 如果发现（匹配）到了新的设备那么 probe 函数就会被调用，并且将这个设备传给这个函数
    // 应该是可重入的（？？），因为内核是多线程的，所以这个函数可能会被同时调用
    printk(KERN_INFO "LED Character Platform Device(Driver) is initialized.\n");
    printk(KERN_INFO "LED Character Platform Device(Driver) is probed.\n");
    printk(KERN_INFO "LED Character Platform Device(Driver) matched name: %s\n", pdev->name);

    int ret = 0;

    // allocate led_char_platform structure
    led_char_platform = kmalloc(sizeof(struct led_char_platform), GFP_KERNEL);
    if (!led_char_platform) {
        printk(KERN_ERR "Failed to allocate led_char_platform\n");
        return -ENOMEM;
    }
    memset(led_char_platform, 0, sizeof(struct led_char_platform));

    // get device tree node
    led_char_platform->np = of_find_node_by_name(NULL, "opendev_platform_led");
    if (!led_char_platform->np) {
        printk(KERN_ERR "Failed to find device tree node\n");
        kfree(led_char_platform);
        return -ENODEV;
    }

    // initialize led gpio
    ret = led_char_platform_init_led(led_char_platform);
    if (ret < 0) {
        printk(KERN_ERR "Failed to initialize led_char_platform_led\n");
        kfree(led_char_platform);
        return ret;
    }

    // register character device
    ret = led_char_platform_cdev_init(led_char_platform);
    if (ret < 0) {
        printk(KERN_ERR "Failed to initialize led_char_platform_cdev\n");
        kfree(led_char_platform);
        return ret;
    }

    return ret;
}

static void led_char_platform_remove(struct platform_device *pdev) {
    printk(KERN_INFO "LED Character Platform Device(Driver) is exited\n");
    // unregister character device
    led_char_platform_cdev_exit(led_char_platform);
    // free led_char_platform structure
    kfree(led_char_platform);
}

static const struct of_device_id led_char_platform_dt_ids[] = {
    { .compatible = "opendev-platform-led", },
    { /* sentinel 标记；哨兵 */ }
};

struct platform_driver led_char_platform_driver = {
    .probe = led_char_platform_probe,
    .remove = led_char_platform_remove,
    .driver = {
        // 注意 name 字段和在 platform device 的文件里面设置的 设备 name 字段一致
        .name = "opendev-platform-led",
        .owner = THIS_MODULE,
        .of_match_table = led_char_platform_dt_ids,
    },
};

/*
 * Module initialization and cleanup functions
 */
static int __init led_char_platform_init(void) {
    printk(KERN_INFO "LED Character Platform Device(Driver) is registered.\n");
    return platform_driver_register(&led_char_platform_driver);
}

static void __exit led_char_platform_exit(void) {
    printk(KERN_INFO "LED Character Platform Device(Driver) is unregistered.\n");
    platform_driver_unregister(&led_char_platform_driver);
}

module_init(led_char_platform_init);
module_exit(led_char_platform_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("liyunfeng <1193230388@qq.com>");
MODULE_DESCRIPTION("LED Character Platform Device Driver");
MODULE_VERSION("0.1");
