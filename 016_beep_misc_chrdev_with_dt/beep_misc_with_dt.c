#include <linux/init.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include "linux/minmax.h"
#include "linux/of.h"
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/device/class.h>
#include <linux/of_gpio.h>

struct beep_misc_dev {
    int major;                  // 主设备号
    int minor;                  // 次设备号
    dev_t dev_t;                // 设备号
    struct cdev cdev;           // cdev结构体
    struct class *cls;          // 类
    struct device *dev;         // 设备
    struct device_node *np;     // 设备树节点
    int beep_gpio;              // BEEP 对应的GPIO
};
static struct beep_misc_dev *beep_misc_dev = NULL;

static int beep_misc_dev_open (struct inode *inode, struct file *filp) {
    printk(KERN_INFO "Beep misc Character Device is opened\n");
    // 保存这个 beep_misc_dev 结构体指针，以便其他函数使用
    // 如果不这么做，其他函数无法访问这个结构体，或者多个设备文件同时打开时，无法区分是哪个设备文件
    filp->private_data = beep_misc_dev;
    return 0;
}

static int beep_misc_dev_release (struct inode *inode, struct file *filp) {
    printk(KERN_INFO "Beep misc Character Device is closed\n");
    return 0;
}

static ssize_t beep_misc_dev_read (struct file *filp, char *buff, size_t len, loff_t *loff_t) {
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

static ssize_t beep_misc_dev_write (struct file *filp, const char *buff, size_t len, loff_t *loff_t) {
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
        gpio_set_value(beep_misc_dev->beep_gpio, 0);
    } else if (strcmp(kbuf, "off") == 0) {
        gpio_set_value(beep_misc_dev->beep_gpio, 1);
    } else {
        gpio_set_value(beep_misc_dev->beep_gpio, !gpio_get_value(beep_misc_dev->beep_gpio));
    }
    return ret;
}

static struct file_operations beep_misc_fops = {
    .owner = THIS_MODULE,
    .open = beep_misc_dev_open,
    .release = beep_misc_dev_release,
    .read = beep_misc_dev_read,
    .write = beep_misc_dev_write,
};

static struct miscdevice hello_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "opendev-gpio-beep",
    .fops = &beep_misc_fops,
};


static int beep_misc_dev_init (void) {
    printk("Module init.\n");
    int ret;

    // 初始化 misc device
    beep_misc_dev = kzalloc(sizeof(struct beep_misc_dev), GFP_KERNEL);
    if (!beep_misc_dev) {
        printk("Failed to allocate memory for beep_misc_dev.\n");
        return -ENOMEM;
    }

    beep_misc_dev->np = of_find_node_by_path("/opendev_gpio_beep");
    // init gpio
    beep_misc_dev->beep_gpio = of_get_named_gpio(beep_misc_dev->np, "beep-gpios", 0);
    if (beep_misc_dev->beep_gpio < 0) {
        printk("Failed to get gpio.\n");
        return beep_misc_dev->beep_gpio;
    }
    ret = gpio_direction_output(beep_misc_dev->beep_gpio, 1);
    if (ret < 0) {
        printk("Failed to set gpio direction.\n");
        return ret;
    }

    // 注册 misc device
    ret = misc_register(&hello_misc_device);
    // check ret
    if (ret < 0) {
        printk("Failed to register misc device.\n");
        return ret;
    }
    printk("Beep misc dev Registered.\n");
    return 0;
}

static void beep_misc_dev_exit (void) {
    // close beep
    gpio_set_value(beep_misc_dev->beep_gpio, 1);
    misc_deregister(&hello_misc_device);
    printk("Beep misc device unregistered.\n");
    printk("Module exit.\n");
}


// module_misc_device(beep_misc_device);
module_init(beep_misc_dev_init);
module_exit(beep_misc_dev_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("liyunfeng");
MODULE_DESCRIPTION("Beep misc Character Device");
MODULE_VERSION("0.1");
