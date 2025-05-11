#include "linux/interrupt.h"
#include "linux/irqdomain.h"
#include "linux/irqreturn.h"
#include "linux/jiffies.h"
#include "linux/minmax.h"
#include "linux/of.h"
#include "linux/timer.h"
#include "linux/timer_types.h"
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
#include <linux/of_irq.h>

#define KEY_NUM             1
#define VALID_KEY_VALUE     0xF0
#define INVALID_KEY_VALUE   0x00

/**
中断的处理有问题，应该是 定时器 没出发，回调函数没有用
中断申请没有释放成功，gpio 没有释放成功，不能二次申请
 */

struct irq_key_desc {
    int irq;                                // 中断号
    int gpio;                               // GPIO
    unsigned char key_value;                // 按键值
    char irq_name[32];                      // 中断名称
    irqreturn_t (*handler)(int, void *);    // 中断处理函数
};

struct key_char {
    int major;                  // 主设备号
    int minor;                  // 次设备号
    dev_t dev_t;                // 设备号
    struct cdev cdev;           // cdev结构体
    struct class *cls;          // 类
    struct device *dev;         // 设备
    struct device_node *np;     // 设备树节点
    int key_gpio;               // KEY对应的GPIO
    atomic_t key_value;         // 按键值
    atomic_t key_pressed;       // 按键是否按下
    atomic_t key_released;      // 按键是否松开
    struct timer_list timer;    // 定时器
    struct irq_key_desc irq_desc[KEY_NUM];   // 按键的中断描述符数组
    unsigned int cur_key_num;   // 当前按键号
};
static struct key_char *key_char = NULL;

/**
 * @brief 按键中断处理函数，开启定时器，用于按键消抖
 * @param irq 中断号
 * @param dev_id 设备ID
 * @return IRQ_HANDLED
 * @description 当按键按下时，中断处理函数会被调用，将按键值设置为0xF0，并启动定时器
 *              当按键松开时，中断处理函数会被调用，将按键值设置为0x00，并停止定时器
 *              定时器到期后，将按键值设置为0x00，并停止定时器
 */
static irqreturn_t key_irq_handler(int irq, void *dev_id) {
    if (key_char == NULL) {
        printk(KERN_ERR "key_char is NULL\n");
        return IRQ_HANDLED;
    }
    printk("key_irq_handler\n");
    key_char->cur_key_num = 0;
    // key_char->timer.data = (unsigned long)key_char;
    // 定时器时间可以控制按键按下的时间
    mod_timer(&key_char->timer, jiffies + msecs_to_jiffies(300)); // 10ms后触发定时器
    return IRQ_HANDLED;
}

/**
 * @brief 定时器处理函数，用于按键消抖
 */
static void key_timer_handler(struct timer_list *timer) {
    if (key_char == NULL) {
        printk(KERN_ERR "key_char is NULL\n");
        return;
    }

    // unsigned int key_value = gpio_get_value(key_char->key_gpio); // 获取按键值
    unsigned int key_value = 0; // 获取按键值
    unsigned int key_num = key_char->cur_key_num; // 获取当前按键号
    struct irq_key_desc *key_desc = &key_char->irq_desc[key_num]; // 获取当前按键的中断描述符

    // 定时器时间到了，说明按键还是按下的状态，此时读取按键的值就是就是按下的（低）
    key_value = gpio_get_value(key_desc->gpio); // 获取按键值
    if (key_value == 0) {
        // 按键按下
        // 等待按键释放
        // while (gpio_get_value(key_desc->gpio) == 0) ;

        // 按键松开
        // if (key_value == 0) {
            // 按键按下有效
            printk("key %d pressed\n", key_num);
            atomic_set(&key_char->key_value, VALID_KEY_VALUE);
            atomic_set(&key_char->key_pressed, 1);
            atomic_set(&key_char->key_released, 0);
        // }
    } else {
        // atomic_set(&key_char->key_value, 0x80 | key_desc->key_value);
        // 没有按下或状态不稳定
        printk("key %d released\n", key_num);
        atomic_set(&key_char->key_value, INVALID_KEY_VALUE);
        atomic_set(&key_char->key_released, 0);
        atomic_set(&key_char->key_pressed, 0);
    }
}

/**
 * @brief open 的时候初始化按键IO
 */
static int key_io_init(void) {
    int ret = 0;
    if (key_char == NULL) {
        printk(KERN_ERR "key_char is NULL\n");
        return -1;
    }
    if (key_char->np == NULL) {
        printk(KERN_ERR "key_char->np is NULL\n");
        return -1;
    }

    atomic_set(&key_char->key_value, INVALID_KEY_VALUE);
    atomic_set(&key_char->key_pressed, 0);
    atomic_set(&key_char->key_released, 0);

    // 因为不能重复获取？？
    // key_char->key_gpio = of_get_named_gpio(key_char->np, "key-gpios", 0);
    // if (key_char->key_gpio < 0) {
    //     printk(KERN_ERR "Failed to get key gpio\n");
    //     return -1;
    // }
    // printk(KERN_INFO "key gpio: %d\n", key_char->key_gpio);

    // pick gpio
    for (int i = 0; i < KEY_NUM; i++) {
        key_char->irq_desc[i].gpio = of_get_named_gpio(key_char->np, "key-gpios", i);
        if (key_char->irq_desc[i].gpio < 0) {
            printk(KERN_ERR "Failed to get key gpio\n");
            return -1;
        }
    }
    printk(KERN_INFO "key gpio: %d\n", key_char->key_gpio);

    // init gpio
    for (int i = 0; i < KEY_NUM; i++) {
        // memset(&key_char->irq_desc[i], 0, sizeof(struct irq_key_desc));
        sprintf(key_char->irq_desc[i].irq_name, "key_irq_%d", i);
        gpio_request(key_char->irq_desc[i].gpio, key_char->irq_desc[i].irq_name);
        gpio_direction_input(key_char->irq_desc[i].gpio);

        // key_char->irq_desc[i].irq = gpio_to_irq(key_char->irq_desc[i].gpio);
        // change gpio num to logic irq num
        // 这个函数的问题是只能允许一次映射成功，第二次就不行了，是不可重入的，也不会自动清除旧的 irq_domain 映射。
        // 第二次申请报错：irq: type mismatch, failed to map hwirq-18 for gpio@209c000!
        // 因为在函数内部创建了一个映射关系，所以第二次申请时，会报错，因为已经有一个映射关系了
        // 所以这里需要手动清除旧的 irq_domain 映射，然后重新映射
        // key_char->irq_desc[i].irq = irq_of_parse_and_map(key_char->np, i);
        // irq 不在这里做映射，在 probe(init) 函数中做映射，并保存成全局的
        printk(KERN_INFO "key gpio: %d, irq num: %d\n", key_char->irq_desc[i].gpio, key_char->irq_desc[i].irq);
    }

    // init irq
    // use logic irq num to register irq handler
    key_char->irq_desc[0].handler = key_irq_handler;
    key_char->irq_desc[0].key_value = INVALID_KEY_VALUE; // 初始按键值设置为无效值

    for (int i = 0; i < KEY_NUM; i++) {
        ret = request_irq(key_char->irq_desc[i].irq, key_char->irq_desc[i].handler,
                        IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
                        key_char->irq_desc[i].irq_name, key_char);
        // ret = request_irq(key_char->irq_desc[i].irq, key_char->irq_desc[i].handler,
        //                 IRQF_TRIGGER_LOW,
        //                 key_char->irq_desc[i].irq_name, key_char);
        if (ret < 0) {
            printk(KERN_ERR "Failed to request irq\n");
            return -1;
        }
    }

    // init timer
    timer_setup(&key_char->timer, key_timer_handler, 0);

    return ret;
}

static int key_io_exit(void) {
    if (key_char == NULL) {
        printk(KERN_ERR "key_char is NULL\n");
        return -1;
    }
    printk("key_io_exit\n");
    // delete timer
    del_timer(&key_char->timer);

    printk("irq: %d, gpio: %d release.\n", key_char->irq_desc[0].irq, key_char->irq_desc[0].gpio);
    // delete irq and gpio
    for (int i = 0; i < KEY_NUM; i++) {
        free_irq(key_char->irq_desc[i].irq, key_char);
        gpio_free(key_char->irq_desc[i].gpio);
    }

    return 0;
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
    int ret = key_io_exit();
    return ret;
}

static ssize_t key_char_read (struct file *filp, char *buff, size_t len, loff_t *loff_t) {
    char kbuf[32] = "abc";
    int ret;

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
    if (atomic_read(&key_char->key_pressed) == 1) {
        ret = copy_to_user(buff, &key_char->key_value, len);
        if (ret < 0) {
            printk("Failed to copy data to user space.\n");
            return ret;
        }
        atomic_set(&key_char->key_pressed, 0);
        atomic_set(&key_char->key_released, 1);
        atomic_set(&key_char->key_value, INVALID_KEY_VALUE);
    } else {
        // printk("Key is not pressed.\n");
        ret = -EINVAL; // 错误码
        goto error;
    }

error:
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

    return ret;
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
    ret = alloc_chrdev_region(&key_char->dev_t, 0, 1, "key_chrdev");
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
    // key_char->dev = device_create(key_char->cls, NULL, key_char->dev_t, NULL, "key_chrdev_%d", key_char->minor);
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
    key_char->np = of_find_node_by_path("/opendev_gpio_key_intc");
    if (key_char->np == NULL) {
        printk(KERN_ERR "Failed to find node\n");
        kfree(key_char);
        return -1;
    }

    // get irq number from device tree
    // 必须是只能映射一次
    for (int i = 0; i < KEY_NUM; i++) {
        memset(&key_char->irq_desc[i], 0, sizeof(struct irq_key_desc));
        key_char->irq_desc[i].irq = irq_of_parse_and_map(key_char->np, i); // get irq number
        printk("mapped key %d irq: %d\n", i, key_char->irq_desc[i].irq);
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
