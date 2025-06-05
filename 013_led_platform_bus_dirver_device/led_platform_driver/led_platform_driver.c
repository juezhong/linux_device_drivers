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

// register info alrready use platform device's resource define
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

struct led_char_platform {
    int major;                  // 主设备号
    int minor;                  // 次设备号
    dev_t dev_t;                // 设备号
    struct cdev cdev;           // cdev结构体
    struct class *cls;          // 类
    struct device *dev;         // 设备
    struct led_regs led_regs;   // led寄存器
    struct resource **res;      // 设备中的资源（一些信息）
    resource_size_t *res_size;  // 资源大小
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
    int val = 0;
    switch (status) {
        case LED_ON:
            val = readl(led_char_platform->led_regs.led_reg_GPIO1_DR);
            val &= ~(1 << 3);
            writel(val, led_char_platform->led_regs.led_reg_GPIO1_DR);
            break;
        case LED_OFF:
            val = readl(led_char_platform->led_regs.led_reg_GPIO1_DR);
            val |= (1 << 3);
            writel(val, led_char_platform->led_regs.led_reg_GPIO1_DR);
            break;
        default:
            // switch led status, on->off, off->on
            val = readl(led_char_platform->led_regs.led_reg_GPIO1_DR);
            val ^= (1 << 3);
            writel(val, led_char_platform->led_regs.led_reg_GPIO1_DR);
            break;
    }
    // if (status == LED_ON) {
    //     val = readl(led_char_platform->led_regs.led_reg_GPIO1_DR);
    //     val &= ~(1 << 3);
    //     writel(val, led_char_platform->led_regs.led_reg_GPIO1_DR);
    // } else {
    //     val = readl(led_char_platform->led_regs.led_reg_GPIO1_DR);
    //     val |= (1 << 3);
    //     writel(val, led_char_platform->led_regs.led_reg_GPIO1_DR);
    // }
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

static int led_char_platform_init_regs (struct led_char_platform *led_char_platform) {
    // led_char_platform->led_regs.led_reg_IMX6U_CCM_CCGR1 = ioremap(CCM_CCGR1_BASE_ADDR, 4);
    // led_char_platform->led_regs.led_reg_SW_MUX_GPIO1_IO03 = ioremap(SW_MUX_GPIO1_IO03_BASE_ADDR, 4);
    // led_char_platform->led_regs.led_reg_SW_PAD_GPIO1_IO03 = ioremap(SW_PAD_GPIO1_IO03_BASE_ADDR, 4);
    // led_char_platform->led_regs.led_reg_GPIO1_DR = ioremap(GPIO1_DR_BASE_ADDR, 4);
    // led_char_platform->led_regs.led_reg_GPIO1_GDIR = ioremap(GPIO1_GDIR_BASE_ADDR, 4);

    led_char_platform->led_regs.led_reg_IMX6U_CCM_CCGR1 = ioremap(led_char_platform->res[0]->start, led_char_platform->res_size[0]);
    led_char_platform->led_regs.led_reg_SW_MUX_GPIO1_IO03 = ioremap(led_char_platform->res[1]->start, led_char_platform->res_size[1]);
    led_char_platform->led_regs.led_reg_SW_PAD_GPIO1_IO03 = ioremap(led_char_platform->res[2]->start, led_char_platform->res_size[2]);
    led_char_platform->led_regs.led_reg_GPIO1_DR = ioremap(led_char_platform->res[3]->start, led_char_platform->res_size[3]);
    led_char_platform->led_regs.led_reg_GPIO1_GDIR = ioremap(led_char_platform->res[4]->start, led_char_platform->res_size[4]);

    if (!led_char_platform->led_regs.led_reg_IMX6U_CCM_CCGR1 ||
        !led_char_platform->led_regs.led_reg_SW_MUX_GPIO1_IO03 ||
        !led_char_platform->led_regs.led_reg_SW_PAD_GPIO1_IO03 ||
        !led_char_platform->led_regs.led_reg_GPIO1_DR ||
        !led_char_platform->led_regs.led_reg_GPIO1_GDIR) {
        printk(KERN_ERR "Failed to map led_char_platform_regs\n");
        return -1;
    }

    return 0;
}

static void led_char_platform_exit_regs (struct led_char_platform *led_char_platform) {
    iounmap(led_char_platform->led_regs.led_reg_IMX6U_CCM_CCGR1);
    iounmap(led_char_platform->led_regs.led_reg_SW_MUX_GPIO1_IO03);
    iounmap(led_char_platform->led_regs.led_reg_SW_PAD_GPIO1_IO03);
    iounmap(led_char_platform->led_regs.led_reg_GPIO1_DR);
    iounmap(led_char_platform->led_regs.led_reg_GPIO1_GDIR);
}

static int led_char_platform_init_led (struct led_char_platform *led_char_platform) {
    int val = 0;
    if (led_char_platform == NULL) {
        printk(KERN_ERR "led_char_platform is NULL\n");
        return -1;
    }

    // enable GPIO1 clk
    val = ioread32(led_char_platform->led_regs.led_reg_IMX6U_CCM_CCGR1);
    val &= ~(3 << 26); // Clear bits 26-27 before setting
    val |= (3 << 26);  // Set bits 26-27 to enable clock
    // set GPIO1_IO03 MUX as GPIO
    iowrite32(5, led_char_platform->led_regs.led_reg_SW_MUX_GPIO1_IO03);
    // set GPIO1_IO03 PAD as GPIO
    iowrite32(0x10B0, led_char_platform->led_regs.led_reg_SW_PAD_GPIO1_IO03);
    // set GPIO1_IO03 as output
    val = ioread32(led_char_platform->led_regs.led_reg_GPIO1_GDIR);
    val &= ~(1 << 3); // Clear bit 3 before setting
    val |= (1 << 3);  // Set bit 3 to enable output
    iowrite32(val, led_char_platform->led_regs.led_reg_GPIO1_GDIR);
    // default off led
    val = ioread32(led_char_platform->led_regs.led_reg_GPIO1_DR);
    val |= (1 << 3); // Set bit 3 to turn off led
    iowrite32(val, led_char_platform->led_regs.led_reg_GPIO1_DR);

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

    // allocate led_char_platform->res structure
    led_char_platform->res = kmalloc(sizeof(struct resource) * pdev->num_resources, GFP_KERNEL);
    led_char_platform->res_size = kmalloc(sizeof(resource_size_t) * pdev->num_resources, GFP_KERNEL);
    // get platform data(register base address) from platform device resource
    for (int i = 0; i < pdev->num_resources; i++) {
        led_char_platform->res[i] = platform_get_resource(pdev, IORESOURCE_MEM, i);
        if (!led_char_platform->res[i]) {
            printk(KERN_ERR "Failed to get resource\n");
            kfree(led_char_platform->res);
            kfree(led_char_platform);
            return -ENODEV;
        }
        // led_char_platform->res_size = led_char_platform->res[i]->end - led_char_platform->res[i]->start + 1;
        led_char_platform->res_size[i] = resource_size(led_char_platform->res[i]);
    }

    // remap led register
    ret = led_char_platform_init_regs(led_char_platform);
    if (ret < 0) {
        printk(KERN_ERR "Failed to initialize led_char_platform_regs\n");
        kfree(led_char_platform);
        return ret;
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
    // unmap led register
    led_char_platform_exit_regs(led_char_platform);
}

struct platform_driver led_char_platform_driver = {
    .probe = led_char_platform_probe,
    .remove = led_char_platform_remove,
    .driver = {
        // 注意 name 字段和在 platform device 的文件里面设置的 设备 name 字段一致
        .name = "opendev-platform-led",
        .owner = THIS_MODULE,
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
