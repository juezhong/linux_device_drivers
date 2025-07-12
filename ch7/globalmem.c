/*
 * a simple char device driver: global memory with mutex
 */

#include "linux/mutex.h"
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include <linux/init.h>

#define GLOBAL_MEM_SIZE 0x1000
#define MEM_CLEAR 0x1

/** not use global major dev number and param, use allocate */

struct global_mem_dev {
	struct cdev cdev; // cdev structure
	unsigned char private_data[GLOBAL_MEM_SIZE]; // private data
	/*
	 * or use `u8 private_data[GLOBAL_MEM_SIZE];` ?
	 * yes, it is ok, but need include <linux/types.h>
	 */
	dev_t devno; // device number
	struct class *class; // device class, to create sysfs entry
	struct device *device; // device structure, to create device node
	struct mutex global_mem_mutex;
};

struct global_mem_dev *global_mem_devp;

/*
 * define file operations
 */
static int global_mem_open(struct inode *inode, struct file *filp)
{
	filp->private_data = global_mem_devp; // set private data
	return 0;
}

static int global_mem_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static long global_mem_ioctl(struct file *filp, unsigned int cmd,
			     unsigned long arg)
{
	struct global_mem_dev *dev = filp->private_data;

	switch (cmd) {
	case MEM_CLEAR:
		mutex_lock(&dev->global_mem_mutex);
		// clear the memory
		memset(dev->private_data, 0, GLOBAL_MEM_SIZE);
		printk(KERN_INFO "globalmem is set to zero\n");
		mutex_unlock(&dev->global_mem_mutex);
		break;
	default:
		return -EINVAL; // invalid argument
	}
	return 0;
}

static ssize_t global_mem_read(struct file *filp, char __user *buf,
			       size_t count, loff_t *ppos)
{
	struct global_mem_dev *dev = filp->private_data;
	size_t max_bytes = GLOBAL_MEM_SIZE - *ppos;

	if (*ppos >= GLOBAL_MEM_SIZE)
		return 0; // end of file

	if (count > max_bytes)
		count = max_bytes;

	mutex_lock(&dev->global_mem_mutex);
	if (copy_to_user(buf, dev->private_data + *ppos, count))
		return -EFAULT;

	*ppos += count;
	printk("read %lu bytes from position %lld to %lld\n", count,
	       *ppos - count, *ppos);
	mutex_unlock(&dev->global_mem_mutex);
	return count;
}

static ssize_t global_mem_write(struct file *filp, const char __user *buf,
				size_t count, loff_t *ppos)
{
	struct global_mem_dev *dev = filp->private_data;
	size_t max_bytes = GLOBAL_MEM_SIZE - *ppos;

	if (*ppos >= GLOBAL_MEM_SIZE)
		return 0; // end of file

	if (count > max_bytes)
		count = max_bytes;

	mutex_lock(&dev->global_mem_mutex);
	if (copy_from_user(dev->private_data + *ppos, buf, count))
		return -EFAULT;

	*ppos += count;
	printk("written %lu bytes from position %lld to %lld\n", count,
	       *ppos - count, *ppos);
	mutex_unlock(&dev->global_mem_mutex);
	return count;
}

static loff_t global_mem_llseek(struct file *filp, loff_t offset, int whence)
{
	loff_t newpos;

	switch (whence) {
	case 0: // SEEK_SET
		if (offset < 0 || offset > GLOBAL_MEM_SIZE)
			return -EINVAL; // invalid argument
		filp->f_pos = offset;
		newpos = filp->f_pos;
		break;
	case 1: // SEEK_CUR
		if (filp->f_pos + offset < 0 ||
		    filp->f_pos + offset > GLOBAL_MEM_SIZE)
			return -EINVAL; // invalid argument
		filp->f_pos += offset;
		newpos = filp->f_pos;
		break;
	default:
		return -EINVAL; // invalid argument
		break;
	}

	return newpos;
}

static const struct file_operations global_mem_fops = {
	.owner = THIS_MODULE,
	.open = global_mem_open,
	.release = global_mem_release,
	.read = global_mem_read,
	.write = global_mem_write,
	.unlocked_ioctl = global_mem_ioctl,
	.llseek = global_mem_llseek,
};

static int global_mem_setup_cdev(struct global_mem_dev *dev, int index)
{
	int err;
	// dev_t devno = MKDEV(0, index); // use dynamic major number
	dev_t devno = dev->devno;
	dev_t major, minor;
	if (devno == 0) {
		printk(KERN_ERR "Device number is not set\n");
		return -ENODEV; // device number not set
	}
	major = MAJOR(devno);
	minor = MINOR(devno);
	if (major == 0 || minor < 0) {
		printk(KERN_ERR "Invalid device number: major %d, minor %d\n",
		       major, minor);
		return -EINVAL; // invalid argument
	}
	printk(KERN_INFO "Registering globalmem%d with major %d, minor %d\n",
	       index, major, minor);

	// init cdev
	cdev_init(&dev->cdev, &global_mem_fops);
	dev->cdev.owner = THIS_MODULE;
	// dev->cdev.ops = &global_mem_fops;

	// add cdev to kernel
	err = cdev_add(&dev->cdev, devno, 1);
	if (err) {
		printk(KERN_NOTICE "Error %d adding globalmem%d", err, index);
		return err;
	}

	// create device class
	dev->class = class_create("globalmem_class");
	if (IS_ERR(dev->class)) {
		printk(KERN_ERR "Failed to create device class\n");
		cdev_del(&dev->cdev);
		return PTR_ERR(dev->class);
	}

	// create device node
	dev->device = device_create(dev->class, NULL, devno, NULL,
				    "globalmem%d", index);
	if (IS_ERR(dev->device)) {
		printk(KERN_ERR "Failed to create device node\n");
		class_destroy(dev->class);
		cdev_del(&dev->cdev);
		return PTR_ERR(dev->device);
	}

	return 0;
}

static int __init global_mem_init(void)
{
	int ret;
	dev_t devno;

	if (global_mem_devp) {
		printk(KERN_ERR "global_mem_devp is already initialized\n");
		return -EBUSY; // device already initialized
	}

	// allocate device number dynamically
	global_mem_devp = kmalloc(sizeof(struct global_mem_dev), GFP_KERNEL);
	if (!global_mem_devp) {
		unregister_chrdev_region(devno, 1);
		printk(KERN_ERR
		       "Failed to allocate memory for global_mem_dev\n");
		return -ENOMEM;
	}

	// allocate device number dynamically
	ret = alloc_chrdev_region(&devno, 0, 1, "globalmem");
	if (ret < 0) {
		printk(KERN_ERR "Failed to allocate char device region\n");
		return ret;
	}

	/** set device number */
	global_mem_devp->devno = devno;
	/** clear memory */
	memset(global_mem_devp->private_data, 0, GLOBAL_MEM_SIZE);

	ret = global_mem_setup_cdev(global_mem_devp, 0);
	if (ret) {
		kfree(global_mem_devp);
		unregister_chrdev_region(devno, 1);
		return ret;
	}

	// init mutex
	mutex_init(&global_mem_devp->global_mem_mutex);

	printk(KERN_INFO "Global memory device initialized\n");
	return 0;
}
module_init(global_mem_init);

static void __exit global_mem_exit(void)
{
	if (global_mem_devp) {
		device_destroy(global_mem_devp->class, global_mem_devp->devno);
		class_destroy(global_mem_devp->class);
		cdev_del(&global_mem_devp->cdev);
		kfree(global_mem_devp);
		global_mem_devp = NULL;
		printk(KERN_INFO "Global memory device exited\n");
	} else {
		printk(KERN_WARNING
		       "Global memory device was not initialized\n");
	}
}
module_exit(global_mem_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("liyunfeng <1193230388@qq.com>");
MODULE_DESCRIPTION("A simple char device driver: global memory without mutex");
MODULE_VERSION("0.1");
