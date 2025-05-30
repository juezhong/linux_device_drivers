// System Register Table
//
// ADDR (HEX) -- REGISTER NAME -- DESCRIPTION
// 0x00 -- System Configuration -- Control of basic functions
// 0x01 -- Interrupt Status -- ALS and PS interrupt status output
// 0x02 -- INT Clear Manner -- Auto/semi clear INT pin selector
// 0x0A -- IR Data Low -- Lower byte for IR ADC channel output
// 0x0B -- IR Data High -- Higher byte for IR ADC channel output
// 0x0C -- ALS Data Low -- Lower byte for ALS ADC channel output
// 0x0D -- ALS Data High -- Higher byte for ALS ADC channel output
// 0x0E -- PS Data Low -- Lower byte for PS ADC channel output
// 0x0F -- PS Data High -- Higher byte for PS ADC channel output

#define AP3216C_I2C_ADDR 0x1E

#define AP3216C_SYSTEM_CONFIGURATION    0x00
#define AP3216C_INTERRUPT_STATUS        0x01
#define AP3216C_INT_CLEAR_MANNER        0x02
#define AP3216C_IR_DATA_LOW             0x0A
#define AP3216C_IR_DATA_HIGH            0x0B
#define AP3216C_ALS_DATA_LOW            0x0C
#define AP3216C_ALS_DATA_HIGH           0x0D
#define AP3216C_PS_DATA_LOW             0x0E
#define AP3216C_PS_DATA_HIGH            0x0F

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
    unsigned int ir, als, ps;           // IR, ALS, PS data
};
static struct oled_dev *ap3216c_dev = NULL;


// static void ee1004_probe_temp_sensor(struct i2c_client *client)
// {
// 	struct i2c_board_info info = { .type = "jc42" };
// 	unsigned short addr = 0x18 | (client->addr & 7);
// 	unsigned short addr_list[] = { addr, I2C_CLIENT_END };
// 	int ret;
//
// 	i2c_new_scanned_device(client->adapter, &info, addr_list, NULL);
// }


static int ap3216c_write_regs(struct oled_dev *ap3216c_dev, u8 reg, u8 *buf, u8 len) {
    int ret = 0;
    unsigned char send_buf[32] = {0};
    struct i2c_client *client = ap3216c_dev->client;
    struct i2c_msg msgs;

    send_buf[0] = reg; // register address
    // copy data to buf
    memcpy(&send_buf[1], buf, len);

    msgs.addr = client->addr; // iic address
    msgs.flags = 0;           // write flag
    msgs.len = len + 1;       // register address + data length
    msgs.buf = send_buf;

    // check client
    if (!client) {
        printk(KERN_INFO "ap3216c_ write_regs client is null\n");
        return -1;
    }
    // check adapter
    if (!client->adapter) {
        printk(KERN_INFO "ap3216c_write_regs adapter is null\n");
        return -1;
    }
    // dump msgs
    printk(KERN_INFO "ap3216c_write_regs msgs.addr = 0x%x, msgs.flags = 0x%x, msgs.len = %d\n", msgs.addr, msgs.flags, msgs.len);
    // dump msgs buf
    for (int i = 0; i < msgs.len; i++) {
        printk(KERN_INFO "ap3216c_write_regs msgs.buf[%d] = 0x%x\n", i, msgs.buf[i]);
    }
    ret = i2c_transfer(client->adapter, &msgs, 1);
    if (ret != 1) {
        printk(KERN_INFO "ap3216c_write_regs i2c_transfer failed\n");
    }
    return ret;
}

static int ap3216c_read_regs(struct oled_dev *ap3216c_dev, u8 reg, u8 *buf, u8 len) {
    int ret = 0;
    struct i2c_client *client = ap3216c_dev->client;
    unsigned char send_buf[32] = {0};
    unsigned char recv_buf[32] = {0};
    struct i2c_msg msgs[2];

    // send register address with write flag
    send_buf[0] = reg;
    msgs[0].addr = client->addr;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = send_buf;

    // read data with read flag
    msgs[1].addr = client->addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = len;
    msgs[1].buf = recv_buf;

    ret = i2c_transfer(client->adapter, msgs, 2);
    if (ret == 2) {
        memcpy(buf, recv_buf, len);
    } else {
        printk(KERN_INFO "ap3216c_read_regs i2c_transfer failed\n");
    }
    return ret;
}

static int ap3216c_read_reg(struct oled_dev *ap3216c_dev, u8 reg, u8 *buf) {
    int ret = 0;
    ret = ap3216c_read_regs(ap3216c_dev, reg, buf, 1);
    return ret;
}

static int ap3216c_write_reg(struct oled_dev *ap3216c_dev, u8 reg, u8 *buf) {
    int ret = 0;
    printk("%d %s\n", __LINE__, __func__);
    ret = ap3216c_write_regs(ap3216c_dev, reg, buf, 1);
    return ret;
}

static void ap3216c_get_data(struct oled_dev *ap3216c_dev) {
    u8 buf[6] = {0};
    for (int i = 0; i < 6; i++) {
        ap3216c_read_reg(ap3216c_dev, AP3216C_IR_DATA_LOW + i, &buf[i]);
    }

    // read ir
    if(buf[0] & 0X80) /* IR_OF 位为 1,则数据无效*/
        ap3216c_dev->ir = 0;
    else
        ap3216c_dev->ir = ((unsigned short)buf[1] << 2) | (buf[0] & 0X03); /* 读取 IR 传感器的数据 */

    // read als
    ap3216c_dev->als = ((unsigned short)buf[3] << 8) | buf[2]; /* ALS 数据 */

    // read ps
    if(buf[4] & 0x40) /* IR_OF 位为 1,则数据无效 */
        ap3216c_dev->ps = 0;
    else
        ap3216c_dev->ps = ((unsigned short)(buf[5] & 0X3F) << 4) | (buf[4] & 0X0F); /* 读取 PS 传感器的数据 */
}

static ssize_t ap3216c_read(struct file *flip, char __user *buff, size_t len, loff_t *loff_t) {
    if (ap3216c_dev == NULL) {
        printk(KERN_INFO "ap3216c_dev is NULL\n");
        return -1;
    }
    int ret = 0;
    int data[3] = {0};

    ap3216c_get_data(ap3216c_dev);
    data[0] = ap3216c_dev->ir;
    data[1] = ap3216c_dev->als;
    data[2] = ap3216c_dev->ps;
    ret = copy_to_user(buff, data, sizeof(data));
    if (ret < 0) {
        printk(KERN_INFO "copy_to_user failed\n");
        return -1;
    }

    return ret;
}

static ssize_t ap3216c_write(struct file *flip, const char __user *buff, size_t len, loff_t *loff_t) {
    if (ap3216c_dev == NULL) {
        printk(KERN_INFO "ap3216c_dev is NULL\n");
        return -1;
    }
    return 0;
}

static int ap3216c_open(struct inode *inode, struct file *file) {
    if (ap3216c_dev == NULL) {
        printk(KERN_INFO "ap3216c_dev is NULL\n");
        return -1;
    }
    // set private data
    file->private_data = ap3216c_dev;
    // init ap3216c
    // write system configuration register
    u8 buf = 0x04;
    ap3216c_write_reg(ap3216c_dev, AP3216C_SYSTEM_CONFIGURATION, &buf);
    mdelay(50);
    buf = 0x03;
    ap3216c_write_reg(ap3216c_dev, AP3216C_SYSTEM_CONFIGURATION, &buf);
    return 0;
}

static int ap3216c_release(struct inode *inode, struct file *file) {
    return 0;
}

static struct file_operations ap3216c_char_fops = {
    .owner = THIS_MODULE,
    .open = ap3216c_open,
    .read = ap3216c_read,
    .write = ap3216c_write,
    .release = ap3216c_release,
};

static int ap3216c_cdev_init(struct oled_dev *ap3216c_dev) {
    int ret = 0;
    if (ap3216c_dev == NULL) {
        printk(KERN_INFO "ap3216c_dev is NULL\n");
        return -1;
    }
    // allocate major and minor
    ret = alloc_chrdev_region(&ap3216c_dev->dev_t, 0, 1, "ap3216c_chrdev");
    if (ret < 0) {
        printk(KERN_ERR "Failed to allocate major and minor\n");
        return ret;
    }
    // get major and minor
    ap3216c_dev->major = MAJOR(ap3216c_dev->dev_t);
    ap3216c_dev->minor = MINOR(ap3216c_dev->dev_t);
    printk(KERN_INFO "IIC %s Character Device major: %d, minor: %d\n", ap3216c_dev->name, ap3216c_dev->major, ap3216c_dev->minor);
    // init cdev
    cdev_init(&ap3216c_dev->cdev, &ap3216c_char_fops);
    // add cdev to kernel
    ret = cdev_add(&ap3216c_dev->cdev, ap3216c_dev->dev_t, 1);
    if (ret < 0) {
        printk(KERN_ERR "Failed to add cdev\n");
        unregister_chrdev_region(ap3216c_dev->dev_t, 1);
        return ret;
    }
    // create class and device
    ap3216c_dev->class = class_create( "ap3216c_class");
    if (IS_ERR(ap3216c_dev->class)) {
        printk(KERN_ERR "Failed to create class\n");
        cdev_del(&ap3216c_dev->cdev);
        unregister_chrdev_region(ap3216c_dev->dev_t, 1);
        return PTR_ERR(ap3216c_dev->class);
    }
    ap3216c_dev->device = device_create(ap3216c_dev->class, NULL, ap3216c_dev->dev_t, NULL, ap3216c_dev->name);
    // led_char->dev = device_create(led_char->cls, NULL, led_char->dev_t, NULL, "led_chrdev_%d", led_char->minor);
    if (IS_ERR(ap3216c_dev->device)) {
        printk(KERN_ERR "Failed to create device\n");
        class_destroy(ap3216c_dev->class);
        cdev_del(&ap3216c_dev->cdev);
        unregister_chrdev_region(ap3216c_dev->dev_t, 1);
        return PTR_ERR(ap3216c_dev->device);
    }

    return ret;
}

static void ap3216c_cdev_exit(struct oled_dev *ap3216c_dev) {
    if (ap3216c_dev == NULL) {
        printk(KERN_INFO "ap3216c_dev is NULL\n");
        return;
    }
    device_destroy(ap3216c_dev->class, ap3216c_dev->dev_t); // destroy device
    class_destroy(ap3216c_dev->class); // destroy class
    cdev_del(&ap3216c_dev->cdev); // delete cdev
    unregister_chrdev_region(ap3216c_dev->dev_t, 1); // unregister chrdev
}

static int ap3216c_iic_probe(struct i2c_client *client) {
    int ret = 0;
    printk(KERN_INFO "I2C AP3216C Platform Device(Driver) is probed.\n");
    if (ap3216c_dev == NULL) {
        printk(KERN_INFO "I2C AP3216C Platform Device(Driver) is initializing failed.\n");
        return -1;
    }
    ap3216c_dev->client = client;
    // register_chrdev(unsigned int major, const char *name, const struct file_operations *fops);
    ret = ap3216c_cdev_init(ap3216c_dev);
    return ret;

}
static void ap3216c_iic_remove(struct i2c_client *client) {
    printk(KERN_INFO "I2C AP3216C Platform Device(Driver) is removed.\n");
    ap3216c_cdev_exit(ap3216c_dev);
    if (ap3216c_dev->name) {
        kfree(ap3216c_dev->name); // free name
        ap3216c_dev->name = NULL;
    }
    if (ap3216c_dev) {
        kfree(ap3216c_dev); // free ap3216c_dev
        ap3216c_dev = NULL;
    }
}

// device match table
static const struct of_device_id ap3216c_of_match[] = {
    { .compatible = "opendev,ap3216c", },
    {}
};

// device tree match table
static const struct i2c_device_id ap3216c_iic_id[] = {
    {"opendev,ap3216c", 0},
    {}
};

static struct i2c_driver ap3216c_iic_driver = {
    .driver = {
        .name = "ap3216c_iic_driver",
        .owner = THIS_MODULE,
        .of_match_table = ap3216c_of_match,
    },
    .probe = ap3216c_iic_probe,
    .remove = ap3216c_iic_remove,
    .id_table = ap3216c_iic_id,
};

static int ap3216c_add_iic_driver(void) {
    int ret = 0;
    ret = i2c_add_driver(&ap3216c_iic_driver);
    return ret;
}

static void ap3216c_del_iic_driver(void) {
    i2c_del_driver(&ap3216c_iic_driver);
}

static int __init iic_ap3216c_platform_init(void) {
    int ret = 0;
    printk(KERN_INFO "I2C AP3216C Platform Device(Driver) is initializing.\n");
    // init ap3216c_dev
    ap3216c_dev = kzalloc(sizeof(struct oled_dev), GFP_KERNEL);
    if (!ap3216c_dev) {
        printk(KERN_INFO "I2C AP3216C Platform Device(Driver) is initializing failed.\n");
        return -ENOMEM;
    }
    // set ap3216c dev name
    ap3216c_dev->name = kzalloc(sizeof(char) * 20, GFP_KERNEL);
    if (!ap3216c_dev->name) {
        printk(KERN_INFO "I2C AP3216C name NULL.\n");
        return -ENOMEM;
    }
    // set name: "ap3216c"
    strcpy(ap3216c_dev->name, "ap3216c");

    ret = ap3216c_add_iic_driver();
    printk(KERN_INFO "I2C AP3216C Platform Device(Driver) is registered.\n");
    return ret;
}

static void __exit iic_ap3216c_platform_exit(void) {
    printk(KERN_INFO "I2C AP3216C Platform Device(Driver) is exiting.\n");
    printk(KERN_INFO "I2C AP3216C Platform Device(Driver) is unregistered.\n");
    ap3216c_del_iic_driver();
    // ap3216c_cdev_exit(ap3216c_dev);
    // if (ap3216c_dev->name) {
    //     kfree(ap3216c_dev->name); // free name
    // }
    // kfree(ap3216c_dev); // free ap3216c_dev
}

module_init(iic_ap3216c_platform_init);
module_exit(iic_ap3216c_platform_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("liyunfeng");
MODULE_DESCRIPTION("I2C AP3216C Platform Device(Driver)");
MODULE_VERSION("0.1");
