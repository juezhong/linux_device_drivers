#include "linux/uaccess.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>

static int hello_misc_open (struct inode *inode, struct file *filp) {
    printk("Hello misc device opened.\n");
    return 0;
}
static int hello_misc_release (struct inode *inode, struct file *filp) {
    printk("Hello misc device released.\n");
    return 0;
}

static ssize_t hello_misc_read (struct file *filp, char __user *ubuff, size_t size, loff_t *loff_t) {
    printk("Hello misc device read.\n");
    char kbuf[5] = "abc";
    int ret;
    ret = copy_to_user(ubuff, kbuf, size);
    if (ret < 0) {
        printk("Failed to copy data to user space.\n");
        return ret;
    }
    return 0;
}
static ssize_t hello_misc_write (struct file *filp, const char __user *ubuff, size_t size, loff_t *loff_t) {
    printk("Hello misc device write.\n");
    char kbuf[5] = {'\0'};
    int ret;
    ret = copy_from_user(kbuf, ubuff, size);
    if (ret < 0) {
        printk("Failed to copy data from user space.\n");
        return ret;
    }
    printk("Write from user data: %s\n", kbuf);
    return ret;
}

/**
 Tips: User space can't transfer data direct to Kernel space, must use below function:
 copy_to_user(void *to, const void *from, unsigned long n); [Transfer kernel data to user space]
 copy_from_user(void *to, const void *from, unsigned long n); [Transfer user data to kernel space]
 e.g:
 User -> Kernel, use copy_from_user(kbuf, ubuf, size);
 Kernel -> User, use copy_to_user(ubuf, kbuf, size);
 */

static struct file_operations hello_misc_fops = {
    .owner = THIS_MODULE,
    .open = hello_misc_open,
    .release = hello_misc_release,
    .read = hello_misc_read,
    .write = hello_misc_write,
};

static struct miscdevice hello_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "hello_misc",
    .fops = &hello_misc_fops,
};


static int misc_init (void) {
    printk("Module init.\n");
    int ret;
    ret = misc_register(&hello_misc_device);
    // check ret
    if (ret < 0) {
        printk("Failed to register misc device.\n");
        return ret;
    }
    printk("Registered misc device.\n");
    return 0;
}

static void misc_exit (void) {
    misc_deregister(&hello_misc_device);
    printk("Misc device unregistered.\n");
    printk("Module exit.\n");
}

module_init(misc_init);
module_exit(misc_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("liyunfeng");
MODULE_DESCRIPTION("misc device example");
MODULE_VERSION("0.1");
