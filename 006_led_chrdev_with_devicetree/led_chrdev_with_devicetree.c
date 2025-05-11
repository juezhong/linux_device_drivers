#include "linux/minmax.h"
#include "linux/of.h"
#include "linux/of_address.h"
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

// already defined in devicetree
// #define CCM_CCGR1_BASE_ADDR         0x020C406C
// #define SW_MUX_GPIO1_IO03_BASE_ADDR 0x020E0068
// #define SW_PAD_GPIO1_IO03_BASE_ADDR 0x020E02F4
// #define GPIO1_DR_BASE_ADDR          0x0209C000
// #define GPIO1_GDIR_BASE_ADDR        0x0209C004

struct led_regs {
    void __iomem *led_reg_IMX6U_CCM_CCGR1;
    void __iomem *led_reg_SW_MUX_GPIO1_IO03;
    void __iomem *led_reg_SW_PAD_GPIO1_IO03;
    void __iomem *led_reg_GPIO1_DR;
    void __iomem *led_reg_GPIO1_GDIR;
};

struct led_char {
    int major;                  // 主设备号
    int minor;                  // 次设备号
    dev_t dev_t;                // 设备号
    struct cdev cdev;           // cdev结构体
    struct class *cls;          // 类
    struct device *dev;         // 设备
    struct led_regs led_regs;   // led寄存器
    struct device_node *np;     // 设备树节点
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

static void led_ctrl_switch (enum LED_STATUS status) {
    int val = 0;
    switch (status) {
        case LED_ON:
            val = readl(led_char->led_regs.led_reg_GPIO1_DR);
            val &= ~(1 << 3);
            writel(val, led_char->led_regs.led_reg_GPIO1_DR);
            break;
        case LED_OFF:
            val = readl(led_char->led_regs.led_reg_GPIO1_DR);
            val |= (1 << 3);
            writel(val, led_char->led_regs.led_reg_GPIO1_DR);
            break;
        default:
            // switch led status, on->off, off->on
            val = readl(led_char->led_regs.led_reg_GPIO1_DR);
            val ^= (1 << 3);
            writel(val, led_char->led_regs.led_reg_GPIO1_DR);
            break;
    }
    // if (status == LED_ON) {
    //     val = readl(led_char->led_regs.led_reg_GPIO1_DR);
    //     val &= ~(1 << 3);
    //     writel(val, led_char->led_regs.led_reg_GPIO1_DR);
    // } else {
    //     val = readl(led_char->led_regs.led_reg_GPIO1_DR);
    //     val |= (1 << 3);
    //     writel(val, led_char->led_regs.led_reg_GPIO1_DR);
    // }
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

static struct file_operations led_char_fops = {
    .owner = THIS_MODULE,
    .open = led_char_open,
    .release = led_char_release,
    .read = led_char_read,
    .write = led_char_write,
};

static int led_char_init_regs (struct led_char *led_char) {
    // replace with devicetree
    // led_char->led_regs.led_reg_IMX6U_CCM_CCGR1 = ioremap(CCM_CCGR1_BASE_ADDR, 4);
    // led_char->led_regs.led_reg_SW_MUX_GPIO1_IO03 = ioremap(SW_MUX_GPIO1_IO03_BASE_ADDR, 4);
    // led_char->led_regs.led_reg_SW_PAD_GPIO1_IO03 = ioremap(SW_PAD_GPIO1_IO03_BASE_ADDR, 4);
    // led_char->led_regs.led_reg_GPIO1_DR = ioremap(GPIO1_DR_BASE_ADDR, 4);
    // led_char->led_regs.led_reg_GPIO1_GDIR = ioremap(GPIO1_GDIR_BASE_ADDR, 4);

    struct property *prop;
    u32 reg_data[10];
    int ret = 0;
    // get 'compatible' property from device tree
    prop = of_find_property(led_char->np, "compatible", NULL);
    if (!prop) {
        printk(KERN_ERR "Failed to find compatible property\n");
        return -1;
    } else {
        printk(KERN_INFO "compatible: %s\n", (char*)prop->value);
    }
    // get 'status' property from device tree
    prop = of_find_property(led_char->np, "status", NULL);
    if (!prop) {
        printk(KERN_ERR "Failed to find status property\n");
        return -1;
    } else {
        printk(KERN_INFO "status: %s\n", (char*)prop->value);
    }
    // get 'reg' property from device tree
    ret = of_property_read_u32_array(led_char->np, "reg", reg_data, 5);
    if (ret < 0) {
        printk(KERN_ERR "Failed to find reg property\n");
        return -1;
    }
    // print reg data, size is 5 groups
    for (int i = 0; i < 10; i++) {
        printk(KERN_INFO "reg[%d]: 0x%x\n", i, reg_data[i]);
    }

    // map registers
#if 0
    led_char->led_regs.led_reg_IMX6U_CCM_CCGR1 = ioremap(reg_data[0], reg_data[1]);
    led_char->led_regs.led_reg_SW_MUX_GPIO1_IO03 = ioremap(reg_data[2], reg_data[3]);
    led_char->led_regs.led_reg_SW_PAD_GPIO1_IO03 = ioremap(reg_data[4], reg_data[5]);
    led_char->led_regs.led_reg_GPIO1_DR = ioremap(reg_data[6], reg_data[7]);
    led_char->led_regs.led_reg_GPIO1_GDIR = ioremap(reg_data[8], reg_data[9]);
#else
    led_char->led_regs.led_reg_IMX6U_CCM_CCGR1 = of_iomap(led_char->np, 0 );
    led_char->led_regs.led_reg_SW_MUX_GPIO1_IO03 = of_iomap(led_char->np, 1 );
    led_char->led_regs.led_reg_SW_PAD_GPIO1_IO03 = of_iomap(led_char->np, 2 );
    led_char->led_regs.led_reg_GPIO1_DR = of_iomap(led_char->np, 3 );
    led_char->led_regs.led_reg_GPIO1_GDIR = of_iomap(led_char->np, 4 );
#endif

    if (!led_char->led_regs.led_reg_IMX6U_CCM_CCGR1 ||
        !led_char->led_regs.led_reg_SW_MUX_GPIO1_IO03 ||
        !led_char->led_regs.led_reg_SW_PAD_GPIO1_IO03 ||
        !led_char->led_regs.led_reg_GPIO1_DR ||
        !led_char->led_regs.led_reg_GPIO1_GDIR) {
        printk(KERN_ERR "Failed to map led_char_regs\n");
        return -1;
    }

    return 0;
}

static void led_char_exit_regs (struct led_char *led_char) {
    iounmap(led_char->led_regs.led_reg_IMX6U_CCM_CCGR1);
    iounmap(led_char->led_regs.led_reg_SW_MUX_GPIO1_IO03);
    iounmap(led_char->led_regs.led_reg_SW_PAD_GPIO1_IO03);
    iounmap(led_char->led_regs.led_reg_GPIO1_DR);
    iounmap(led_char->led_regs.led_reg_GPIO1_GDIR);
}

static int led_char_init_led (struct led_char *led_char) {
    int val = 0;
    if (led_char == NULL) {
        printk(KERN_ERR "led_char is NULL\n");
        return -1;
    }

    // enable GPIO1 clk
    val = ioread32(led_char->led_regs.led_reg_IMX6U_CCM_CCGR1);
    val &= ~(3 << 26); // Clear bits 26-27 before setting
    val |= (3 << 26);  // Set bits 26-27 to enable clock
    // set GPIO1_IO03 MUX as GPIO
    iowrite32(5, led_char->led_regs.led_reg_SW_MUX_GPIO1_IO03);
    // set GPIO1_IO03 PAD as GPIO
    iowrite32(0x10B0, led_char->led_regs.led_reg_SW_PAD_GPIO1_IO03);
    // set GPIO1_IO03 as output
    val = ioread32(led_char->led_regs.led_reg_GPIO1_GDIR);
    val &= ~(1 << 3); // Clear bit 3 before setting
    val |= (1 << 3);  // Set bit 3 to enable output
    iowrite32(val, led_char->led_regs.led_reg_GPIO1_GDIR);
    // default off led
    val = ioread32(led_char->led_regs.led_reg_GPIO1_DR);
    val |= (1 << 3); // Set bit 3 to turn off led
    iowrite32(val, led_char->led_regs.led_reg_GPIO1_DR);

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
    led_char->np = of_find_node_by_path("/opendev_led");
    if (led_char->np == NULL) {
        printk(KERN_ERR "Failed to find node\n");
        kfree(led_char);
        return -1;
    }
    // init led register address from device tree
    ret = led_char_init_regs(led_char);
    if (ret < 0) {
        printk(KERN_ERR "Failed to initialize led_char_regs\n");
        kfree(led_char);
        return ret;
    }

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
    // unmap led register
    led_char_exit_regs(led_char);
}

module_init(led_char_init);
module_exit(led_char_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("liyunfeng");
MODULE_DESCRIPTION("LED Character Device");
MODULE_VERSION("0.1");
