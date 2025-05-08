#include "linux/gpio/consumer.h"
#include "linux/minmax.h"
#include "linux/of.h"
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
#include <linux/of_gpio.h>

#define VALID_KEY_VALUE     0xF0
#define INVALID_KEY_VALUE   0x00

struct key_char {
    int major;                  // 主设备号
    int minor;                  // 次设备号
    dev_t dev_t;                // 设备号
    struct cdev cdev;           // cdev结构体
    struct class *cls;          // 类
    struct device *dev;         // 设备
    struct device_node *np;     // 设备树节点
    int key_gpio;               // LED对应的GPIO
    atomic_t key_value;         // 按键值
};
static struct key_char *key_char = NULL;

/**
 * @brief open 的时候初始化按键IO
 */
static int key_io_init(void)
{
    int ret = 0;
    if (key_char == NULL) {
        printk(KERN_ERR "key_char is NULL\n");
        return -1;
    }
    if (key_char->np == NULL) {
        printk(KERN_ERR "key_char->np is NULL\n");
        return -1;
    }

    key_char->key_gpio = of_get_named_gpio(key_char->np, "key-gpios", 0);
    if (key_char->key_gpio < 0) {
        printk(KERN_ERR "Failed to get key gpio\n");
        return -1;
    }
    printk(KERN_INFO "key gpio: %d\n", key_char->key_gpio);

    gpio_request(key_char->key_gpio, "key_0");
    gpiod_direction_input(gpio_to_desc(key_char->key_gpio));

    return ret;
}

static int key_char_open (struct inode *inode, struct file *filp) {
    printk(KERN_INFO "GPIO Key Character Device is opened\n");
    // 保存这个 key_char 结构体指针，以便其他函数使用
    // 如果不这么做，其他函数无法访问这个结构体，或者多个设备文件同时打开时，无法区分是哪个设备文件
    filp->private_data = key_char;

    int ret = key_io_init();
    if (ret < 0) {
        printk(KERN_ERR "Failed to init key io\n");
        return ret;
    }

    return ret;
}

static int key_char_release (struct inode *inode, struct file *filp) {
    printk(KERN_INFO "GPIO Key Character Device is closed\n");
    return 0;
}

static ssize_t key_char_read (struct file *filp, char *buff, size_t len, loff_t *loff_t) {
    char kbuf[32] = "abc";
    int ret;
    unsigned char value = 0;

    len = MIN(len, 32);
    // len = min(len, 32);
    ret = copy_to_user(buff, kbuf, len);
    if (ret < 0) {
        printk("Failed to copy data to user space.\n");
        return ret;
    }

    if (key_char == NULL) {
        printk(KERN_ERR "key_char is NULL\n");
        return -1;
    }
    // 获取按键值
    // Key value is 0xF0 when key is pressed, 0x00 when key is released
    // Key 按下
    if (gpio_get_value(key_char->key_gpio) == 0) {
        while (!gpio_get_value(key_char->key_gpio)); // 等待按键松开
        atomic_set(&key_char->key_value, VALID_KEY_VALUE);
    } else {
        atomic_set(&key_char->key_value, INVALID_KEY_VALUE); // 无效按键值
    }

    value = atomic_read(&key_char->key_value); // 获取按键值
    // printk(KERN_INFO "Read key value: %d\n", value);
    ret = copy_to_user(buff, &value, sizeof(value));
    return ret;
}

static ssize_t key_char_write (struct file *filp, const char *buff, size_t len, loff_t *loff_t) {
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

    return 0;
}

static struct file_operations key_char_fops = {
    .owner = THIS_MODULE,
    .open = key_char_open,
    .release = key_char_release,
    .read = key_char_read,
    .write = key_char_write,
};



static int key_char_cdev_init (struct key_char *key_char) {
    int ret = 0;
    if (key_char == NULL) {
        printk(KERN_ERR "key_char is NULL\n");
        return -1;
    }
    // allocate major and minor
    ret = alloc_chrdev_region(&key_char->dev_t, 0, 1, "led_chrdev");
    if (ret < 0) {
        printk(KERN_ERR "Failed to allocate major and minor\n");
        return ret;
    }
    key_char->major = MAJOR(key_char->dev_t);
    key_char->minor = MINOR(key_char->dev_t);
    printk(KERN_INFO "GPIO Key Character Device major: %d, minor: %d\n", key_char->major, key_char->minor);

    // init cdev
    key_char->cdev.owner = THIS_MODULE; // must??
    cdev_init(&key_char->cdev, &key_char_fops);
    // add cdev to kernel
    ret = cdev_add(&key_char->cdev, key_char->dev_t, 1);
    if (ret < 0) {
        printk(KERN_ERR "Failed to add cdev\n");
        unregister_chrdev_region(key_char->dev_t, 1);
        return ret;
    }

    // create class and device
    key_char->cls = class_create( "key_class");
    if (IS_ERR(key_char->cls)) {
        printk(KERN_ERR "Failed to create class\n");
        cdev_del(&key_char->cdev);
        unregister_chrdev_region(key_char->dev_t, 1);
        return PTR_ERR(key_char->cls);
    }

    key_char->dev = device_create(key_char->cls, NULL, key_char->dev_t, NULL, "key_chrdev");
    // key_char->dev = device_create(key_char->cls, NULL, key_char->dev_t, NULL, "led_chrdev_%d", key_char->minor);
    if (IS_ERR(key_char->dev)) {
        printk(KERN_ERR "Failed to create device\n");
        class_destroy(key_char->cls);
        cdev_del(&key_char->cdev);
        unregister_chrdev_region(key_char->dev_t, 1);
        return PTR_ERR(key_char->dev);
    }
    printk(KERN_INFO "GPIO Key Character Device name: %s\n", dev_name(key_char->dev));

    return ret;
}

static void key_char_cdev_exit (struct key_char *key_char) {
    if (key_char == NULL) {
        printk(KERN_ERR "key_char is NULL\n");
        return;
    }
    // destroy device and class
    device_destroy(key_char->cls, key_char->dev_t);
    class_destroy(key_char->cls);
    // remove cdev from kernel and unregister major and minor
    cdev_del(&key_char->cdev);
    unregister_chrdev_region(key_char->dev_t, 1);
}

/*
 * Module initialization and cleanup functions
 */
static int __init key_char_init(void) {
    printk(KERN_INFO "GPIO Key Character Device is initialized\n");
    int ret = 0;

    // allocate key_char structure
    key_char = kmalloc(sizeof(struct key_char), GFP_KERNEL);
    if (!key_char) {
        printk(KERN_ERR "Failed to allocate key_char\n");
        return -ENOMEM;
    }
    memset(key_char, 0, sizeof(struct key_char));
    // initialize atomic counter
    atomic_set(&key_char->key_value, INVALID_KEY_VALUE);

    // get poroperty from device tree
    key_char->np = of_find_node_by_path("/opendev_gpio_key");
    if (key_char->np == NULL) {
        printk(KERN_ERR "Failed to find node\n");
        kfree(key_char);
        return -1;
    }

    // register character device
    ret = key_char_cdev_init(key_char);
    if (ret < 0) {
        printk(KERN_ERR "Failed to initialize key_char_cdev\n");
        kfree(key_char);
        return ret;
    }

    return ret;
}

static void __exit key_char_exit(void) {
    printk(KERN_INFO "GPIO Key Character Device is exited\n");
    // unregister character device
    key_char_cdev_exit(key_char);
    // free key_char structure
    kfree(key_char);
}

module_init(key_char_init);
module_exit(key_char_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("liyunfeng");
MODULE_DESCRIPTION("GPIO Key Character Device");
MODULE_VERSION("0.1");
