#include <linux/init.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>

static struct file_operations hello_misc_fops = {
    .owner = THIS_MODULE,
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
MODULE_AUTHOR("liyunfeng <1193230388@qq.com>");
MODULE_DESCRIPTION("A simple misc device example");
MODULE_VERSION("0.1");
