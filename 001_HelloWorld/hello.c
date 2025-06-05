#include <linux/init.h>
#include <linux/module.h>

static int hello_init (void){
    printk("hello world.\n");
    return 0;
}

static void hello_exit (void) {
    printk("bye.\n");
}

module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("liyunfeng <1193230388@qq.com>");
MODULE_DESCRIPTION("Hello World Module");
MODULE_VERSION("0.1");
