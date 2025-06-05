#include "linux/minmax.h"
#include "linux/of.h"
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
#include <linux/of_gpio.h>

struct led_char {
    int major;                  // 主设备号
    int minor;                  // 次设备号
    dev_t dev_t;                // 设备号
    struct cdev cdev;           // cdev结构体
    struct class *cls;          // 类
    struct device *dev;         // 设备
    struct device_node *np;     // 设备树节点
    int led_gpio;               // LED对应的GPIO
};
static struct led_char *led_char = NULL;

enum LED_STATUS {
    LED_OFF = 0,
    LED_ON = 1,
    LED_SWITCH = 2,
};

static int led_char_open (struct inode *inode, struct file *filp) {
    printk(KERN_INFO "LED Character Device is opened\n");
    // 保存这个 led_char 结构体指针，以便其他函数使用
    // 如果不这么做，其他函数无法访问这个结构体，或者多个设备文件同时打开时，无法区分是哪个设备文件
    filp->private_data = led_char;
    return 0;
}

static int led_char_release (struct inode *inode, struct file *filp) {
    printk(KERN_INFO "LED Character Device is closed\n");
    return 0;
}

static ssize_t led_char_read (struct file *filp, char *buff, size_t len, loff_t *loff_t) {
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

static ssize_t led_char_write (struct file *filp, const char *buff, size_t len, loff_t *loff_t) {
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

    if (strcmp(kbuf, "on") == 0) {
        // led_ctrl_switch(LED_ON);
        gpio_set_value(led_char->led_gpio, 0);
    } else if (strcmp(kbuf, "off") == 0) {
        // led_ctrl_switch(LED_OFF);
        gpio_set_value(led_char->led_gpio, 1);
    } else {
        // led_ctrl_switch(LED_SWITCH);
        gpio_set_value(led_char->led_gpio, !gpio_get_value(led_char->led_gpio));
    }
    return ret;
}

static struct file_operations led_char_fops = {
    .owner = THIS_MODULE,
    .open = led_char_open,
    .release = led_char_release,
    .read = led_char_read,
    .write = led_char_write,
};

static int led_char_init_led (struct led_char *led_char) {
    int ret = 0;
    if (led_char == NULL) {
        printk(KERN_ERR "led_char is NULL\n");
        return -1;
    }
    // get gpio number from device tree
    led_char->led_gpio = of_get_named_gpio(led_char->np, "led-gpios", 0);
    if (led_char->led_gpio < 0) {
        printk(KERN_ERR "Failed to get gpio number\n");
        return -1;
    }
    ret = gpio_direction_output(led_char->led_gpio, 1);
    if (ret < 0) {
        printk(KERN_ERR "Failed to set gpio direction\n");
        return ret;
    }
    return 0;
}

static int led_char_cdev_init (struct led_char *led_char) {
    int ret = 0;
    if (led_char == NULL) {
        printk(KERN_ERR "led_char is NULL\n");
        return -1;
    }
    // allocate major and minor
    ret = alloc_chrdev_region(&led_char->dev_t, 0, 1, "led_chrdev");
    if (ret < 0) {
        printk(KERN_ERR "Failed to allocate major and minor\n");
        return ret;
    }
    led_char->major = MAJOR(led_char->dev_t);
    led_char->minor = MINOR(led_char->dev_t);
    printk(KERN_INFO "LED Character Device major: %d, minor: %d\n", led_char->major, led_char->minor);
    // init cdev
    cdev_init(&led_char->cdev, &led_char_fops);
    // add cdev to kernel
    ret = cdev_add(&led_char->cdev, led_char->dev_t, 1);
    if (ret < 0) {
        printk(KERN_ERR "Failed to add cdev\n");
        unregister_chrdev_region(led_char->dev_t, 1);
        return ret;
    }
    // create class and device
    led_char->cls = class_create( "led_class");
    if (IS_ERR(led_char->cls)) {
        printk(KERN_ERR "Failed to create class\n");
        cdev_del(&led_char->cdev);
        unregister_chrdev_region(led_char->dev_t, 1);
        return PTR_ERR(led_char->cls);
    }
    led_char->dev = device_create(led_char->cls, NULL, led_char->dev_t, NULL, "led_chrdev");
    // led_char->dev = device_create(led_char->cls, NULL, led_char->dev_t, NULL, "led_chrdev_%d", led_char->minor);
    if (IS_ERR(led_char->dev)) {
        printk(KERN_ERR "Failed to create device\n");
        class_destroy(led_char->cls);
        cdev_del(&led_char->cdev);
        unregister_chrdev_region(led_char->dev_t, 1);
        return PTR_ERR(led_char->dev);
    }
    printk(KERN_INFO "LED Character Device name: %s\n", dev_name(led_char->dev));

    return ret;
}

static void led_char_cdev_exit (struct led_char *led_char) {
    if (led_char == NULL) {
        printk(KERN_ERR "led_char is NULL\n");
        return;
    }
    // destroy device and class
    device_destroy(led_char->cls, led_char->dev_t);
    class_destroy(led_char->cls);
    // remove cdev from kernel and unregister major and minor
    cdev_del(&led_char->cdev);
    unregister_chrdev_region(led_char->dev_t, 1);
}

/*
 * Module initialization and cleanup functions
 */
static int __init led_char_init(void) {
    printk(KERN_INFO "LED Character Device is initialized\n");
    int ret = 0;

    // allocate led_char structure
    led_char = kmalloc(sizeof(struct led_char), GFP_KERNEL);
    if (!led_char) {
        printk(KERN_ERR "Failed to allocate led_char\n");
        return -ENOMEM;
    }
    memset(led_char, 0, sizeof(struct led_char));

    // get poroperty from device tree
    led_char->np = of_find_node_by_path("/opendev_gpio_led");
    if (led_char->np == NULL) {
        printk(KERN_ERR "Failed to find node\n");
        kfree(led_char);
        return -1;
    }
    // init led gpio from device tree
    // initialize led gpio
    ret = led_char_init_led(led_char);
    if (ret < 0) {
        printk(KERN_ERR "Failed to initialize led_char_led\n");
        kfree(led_char);
        return ret;
    }

    // register character device
    ret = led_char_cdev_init(led_char);
    if (ret < 0) {
        printk(KERN_ERR "Failed to initialize led_char_cdev\n");
        kfree(led_char);
        return ret;
    }

    return ret;
}

static void __exit led_char_exit(void) {
    printk(KERN_INFO "LED Character Device is exited\n");
    // unregister character device
    led_char_cdev_exit(led_char);
    // free led_char structure
    kfree(led_char);
}

module_init(led_char_init);
module_exit(led_char_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("liyunfeng <1193230388@qq.com>");
MODULE_DESCRIPTION("LED Character Device");
MODULE_VERSION("0.1");
