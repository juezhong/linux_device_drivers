#include "linux/stat.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

/**
 Example use:
 insmod module_param.ko param01=3 param02_arr=1,2,3 param03_str="String param"
 Tips:
 Must be consistent with the set parameter name.
 */

static int param01 = 0;
// param_check_int(param01, &param01);
module_param(param01, int, S_IRUGO);
MODULE_PARM_DESC(param01, "desc: Int parameter");

static int param02_arr[3] = {0};
static int param02_arr_size;
module_param_array(param02_arr, int, &param02_arr_size, S_IRUGO);
MODULE_PARM_DESC(param02_arr, "desc: Array parameter");

static char param03_str[256] = {'\0'};
// 1st parameter is param's name, 2nd parameter is param's address.
module_param_string(param03_str, param03_str, sizeof(param03_str), S_IRUGO);
MODULE_PARM_DESC(param03_str, "desc: String parameter.");

static int m_init (void) {
    printk("Module init.\n");
    printk("The param01(int) is %d.\n", param01);

    // print param array size
    printk("The param02_arr_size(int) is %d.\n", param02_arr_size);
    // print param array
    int i = 0;
    for (i = 0; i < param02_arr_size; i++) {
        printk("The param02_arr[%d](int) is %d.\n", i, param02_arr[i]);
    }

    // print param string
    printk("The param03_str(string) is %s.\n", param03_str);

    return 0;
}

static void m_exit (void) {
    printk("Module exit.\n");
}

module_init(m_init);
module_exit(m_exit);
MODULE_LICENSE("GPL");
