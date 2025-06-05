#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/ioctl.h>

#include "iic_oled_cmds.h"

#define OLED_I2C_ADDR 0x3C

struct oled_dev {
    dev_t dev_t;                        // 设备号
    int major;                          // 主设备号
    int minor;                          // 次设备号
    char *name;                         // 设备名
    struct i2c_client *client;          // i2c客户端
    struct i2c_board_info *board_info;  // board info
    struct cdev cdev;                   // 字符设备
    struct class *class;                // 类
    struct device *device;              // 设备
    struct device_node *np;             // 设备树节点
    int x, y;                           // oled坐标
};
static struct oled_dev *oled_dev = NULL;


// static void ee1004_probe_temp_sensor(struct i2c_client *client)
// {
// 	struct i2c_board_info info = { .type = "jc42" };
// 	unsigned short addr = 0x18 | (client->addr & 7);
// 	unsigned short addr_list[] = { addr, I2C_CLIENT_END };
// 	int ret;
//
// 	i2c_new_scanned_device(client->adapter, &info, addr_list, NULL);
// }




static ssize_t oled_read(struct file *flip, char __user *buff, size_t len, loff_t *loff_t) {
    if (oled_dev == NULL) {
        printk(KERN_INFO "oled_dev is NULL\n");
        return -1;
    }
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

static ssize_t oled_write(struct file *flip, const char __user *buff, size_t len, loff_t *loff_t) {
    if (oled_dev == NULL) {
        printk(KERN_INFO "oled_dev is NULL\n");
        return -1;
    }
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
    return ret;
}

static int oled_open(struct inode *inode, struct file *file) {
    if (oled_dev == NULL) {
        printk(KERN_INFO "oled_dev is NULL\n");
        return -1;
    }
    // set private data
    file->private_data = oled_dev;
    // init oled
    return 0;
}

static int oled_release(struct inode *inode, struct file *file) {
    return 0;
}

static long oled_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
    // unsigned long flags;
    int ret = 0;
    struct oled_dev *oled_dev = filp->private_data;
    struct str {
        char *_str;
        int _len;
    };
    struct str kstr;
    char kbuf[65] = {'\0'};

    switch (cmd) {
        case IOCTL_CMD_OLED_TURN_ON_INIT:
            // printk("CMD: IOCTL_CMD_OLED_TURN_ON_INIT\n");
            iic_oled_init(oled_dev->client);
            break;
        case IOCTL_CMD_OLED_TURN_OFF:
            // printk("CMD: IOCTL_CMD_OLED_TURN_OFF\n");
            iic_oled_close(oled_dev->client);
            break;
        case IOCTL_CMD_OLED_SETCURSOR:
            // printk("CMD: IOCTL_CMD_OLED_SETCURSOR\n");
            ssd1306_set_cursor_row(oled_dev->client, arg);
            break;
        case IOCTL_CMD_OLED_CLEAR:
            // printk("CMD: IOCTL_CMD_OLED_CLEAR\n");
            iic_oled_clear(oled_dev->client);
            break;
        case IOCTL_CMD_OLED_WRITE_STR:
            // printk("CMD: IOCTL_CMD_OLED_WRITE_STRING\n");
            for (int i = 0; i < 65; i++) {
                kbuf[i] = '-';
            }
            kbuf[64] = '\0';
            iic_oled_clear(oled_dev->client);
            ssd1306_write_string(oled_dev->client, kbuf);
            mdelay(1000);
            iic_oled_clear(oled_dev->client);
            // check arg is valid
            if (arg) {
                printk(KERN_INFO "arg: %ld\n", arg);
                // copy from user to kstr
                ret = copy_from_user(&kstr, (void *)arg, sizeof(struct str));
                if (ret < 0) {
                    printk(KERN_INFO "Failed to copy from user to kstr\n");
                    return ret;
                }
                if (kstr._len > 65) {
                    printk(KERN_INFO "String length is too long\n");
                    return -1;
                }
                // copy from kstr to kbuf
                ret = copy_from_user(kbuf, kstr._str, kstr._len);
                if (ret < 0) {
                    printk(KERN_INFO "Failed to copy from kstr to kbuf\n");
                    return ret;
                }
                kbuf[kstr._len] = '\0';
                printk(KERN_INFO "String length: %d\n", kstr._len);
                printk(KERN_INFO "String: %s\n", kbuf);
            }
            ssd1306_write_hanzi(oled_dev->client, kbuf);
            break;
        default:
            break;
    }
    return 0;
}

static struct file_operations oled_char_fops = {
    .owner = THIS_MODULE,
    .open = oled_open,
    .read = oled_read,
    .write = oled_write,
    .release = oled_release,
    .unlocked_ioctl = oled_ioctl,
};

static int oled_cdev_init(struct oled_dev *oled_dev) {
    int ret = 0;
    if (oled_dev == NULL) {
        printk(KERN_INFO "oled_dev is NULL\n");
        return -1;
    }
    // allocate major and minor
    ret = alloc_chrdev_region(&oled_dev->dev_t, 0, 1, "oled_chrdev");
    if (ret < 0) {
        printk(KERN_ERR "Failed to allocate major and minor\n");
        return ret;
    }
    // get major and minor
    oled_dev->major = MAJOR(oled_dev->dev_t);
    oled_dev->minor = MINOR(oled_dev->dev_t);
    printk(KERN_INFO "IIC %s Character Device major: %d, minor: %d\n", oled_dev->name, oled_dev->major, oled_dev->minor);
    // init cdev
    cdev_init(&oled_dev->cdev, &oled_char_fops);
    // add cdev to kernel
    ret = cdev_add(&oled_dev->cdev, oled_dev->dev_t, 1);
    if (ret < 0) {
        printk(KERN_ERR "Failed to add cdev\n");
        unregister_chrdev_region(oled_dev->dev_t, 1);
        return ret;
    }
    // create class and device
    oled_dev->class = class_create( "oled_class");
    if (IS_ERR(oled_dev->class)) {
        printk(KERN_ERR "Failed to create class\n");
        cdev_del(&oled_dev->cdev);
        unregister_chrdev_region(oled_dev->dev_t, 1);
        return PTR_ERR(oled_dev->class);
    }
    oled_dev->device = device_create(oled_dev->class, NULL, oled_dev->dev_t, NULL, oled_dev->name);
    // led_char->dev = device_create(led_char->cls, NULL, led_char->dev_t, NULL, "led_chrdev_%d", led_char->minor);
    if (IS_ERR(oled_dev->device)) {
        printk(KERN_ERR "Failed to create device\n");
        class_destroy(oled_dev->class);
        cdev_del(&oled_dev->cdev);
        unregister_chrdev_region(oled_dev->dev_t, 1);
        return PTR_ERR(oled_dev->device);
    }

    return ret;
}

static void oled_cdev_exit(struct oled_dev *oled_dev) {
    if (oled_dev == NULL) {
        printk(KERN_INFO "oled_dev is NULL\n");
        return;
    }
    device_destroy(oled_dev->class, oled_dev->dev_t); // destroy device
    class_destroy(oled_dev->class); // destroy class
    cdev_del(&oled_dev->cdev); // delete cdev
    unregister_chrdev_region(oled_dev->dev_t, 1); // unregister chrdev
}

static int oled_iic_probe(struct i2c_client *client) {
    int ret = 0;
    printk(KERN_INFO "I2C OLED Platform Device(Driver) is probed.\n");
    if (oled_dev == NULL) {
        printk(KERN_INFO "I2C OLED Platform Device(Driver) is initializing failed.\n");
        return -1;
    }
    oled_dev->client = client;
    // register_chrdev(unsigned int major, const char *name, const struct file_operations *fops);
    ret = oled_cdev_init(oled_dev);
    return ret;

}
static void oled_iic_remove(struct i2c_client *client) {
    printk(KERN_INFO "I2C OLED Platform Device(Driver) is removed.\n");
    oled_cdev_exit(oled_dev);
    if (oled_dev->name) {
        kfree(oled_dev->name); // free name
        oled_dev->name = NULL;
    }
    if (oled_dev) {
        kfree(oled_dev); // free oled_dev
        oled_dev = NULL;
    }
}

// device match table
static const struct of_device_id oled_of_match[] = {
    { .compatible = "lyf,oled", },
    {}
};

// device tree match table
static const struct i2c_device_id oled_iic_id[] = {
    {"lyf,oled", 0},
    {}
};

static struct i2c_driver oled_iic_driver = {
    .driver = {
        .name = "oled_iic_driver",
        .owner = THIS_MODULE,
        .of_match_table = oled_of_match,
    },
    .probe = oled_iic_probe,
    .remove = oled_iic_remove,
    .id_table = oled_iic_id,
};

static int oled_add_iic_driver(void) {
    int ret = 0;
    ret = i2c_add_driver(&oled_iic_driver);
    return ret;
}

static void oled_del_iic_driver(void) {
    i2c_del_driver(&oled_iic_driver);
}

static int __init iic_oled_platform_init(void) {
    int ret = 0;
    printk(KERN_INFO "I2C OLED Platform Device(Driver) is initializing.\n");
    // init oled_dev
    oled_dev = kzalloc(sizeof(struct oled_dev), GFP_KERNEL);
    if (!oled_dev) {
        printk(KERN_INFO "I2C OLED Platform Device(Driver) is initializing failed.\n");
        return -ENOMEM;
    }
    // set oled dev name
    oled_dev->name = kzalloc(sizeof(char) * 20, GFP_KERNEL);
    if (!oled_dev->name) {
        printk(KERN_INFO "I2C OLED name NULL.\n");
        return -ENOMEM;
    }
    // set name: "oled"
    strcpy(oled_dev->name, "oled");

    ret = oled_add_iic_driver();
    printk(KERN_INFO "I2C OLED Platform Device(Driver) is registered.\n");
    return ret;
}

static void __exit iic_oled_platform_exit(void) {
    printk(KERN_INFO "I2C OLED Platform Device(Driver) is exiting.\n");
    printk(KERN_INFO "I2C OLED Platform Device(Driver) is unregistered.\n");
    oled_del_iic_driver();
    // oled_cdev_exit(oled_dev);
    // if (oled_dev->name) {
    //     kfree(oled_dev->name); // free name
    // }
    // kfree(oled_dev); // free oled_dev
}

module_init(iic_oled_platform_init);
module_exit(iic_oled_platform_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("liyunfeng <1193230388@qq.com>");
MODULE_DESCRIPTION("I2C OLED Platform Device(Driver)");
MODULE_VERSION("0.1");
