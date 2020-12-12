#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */


#include <linux/uaccess.h>	/* copy_*_user */

#include <linux/kdev_t.h>

#include "cdevice.h"


MODULE_AUTHOR("haruna");
MODULE_LICENSE("Dual BSD/GPL");

int haruDev_major = HARUDEV_MAJOR;
int haruDev_minor = 0;
struct haruDev *mydev;

void trim(struct haruDev *dev) 
{
	struct node *curr = dev->head;
    while(curr) {
		struct node *temp = curr;
		curr = curr->next;
		printk(KERN_INFO "haruDev: freed node\n");
		kfree(temp->data);
		kfree(temp);
	}
	dev->size = 0;
	dev->head = NULL;
}

static int myopen(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "haruDev: invoke myopen()!\n");
	printk(KERN_INFO "haruDev: in inode, major: %d, minor: %d\n", imajor(inode), iminor(inode));
	struct haruDev *dev = container_of(inode->i_cdev, struct haruDev, cdev);
	filp->private_data = dev;

	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		trim(dev);
	}
	return 0;
}

static int myrelease(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "haruDev: invoke myrelease()!\n");
	return 0;
}

struct node* follow(struct haruDev *dev, int item)
{	
	struct node *curr = dev->head;

	/* boundary condition, the first node */
	if (!curr) {
		curr = dev->head = kmalloc(sizeof(struct node), GFP_KERNEL);
		if (!curr) {
			printk(KERN_WARNING "haruDev: kmalloc failed at follow()!\n");
			return NULL;
		}
		memset(curr, 0, sizeof(struct node));
	}

	/* start to follow the list */
	while (item--) {
		if (!curr->next) {
			curr->next = kmalloc(sizeof(struct node), GFP_KERNEL);
			if (!curr->next) {
				printk(KERN_WARNING "haruDev: kmalloc failed at follow()!\n");
				return NULL;
			}
		}
		curr = curr->next; // move to next node;
	}
	return curr;
}

ssize_t mywrite(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	struct haruDev *dev = filp->private_data;

	int item = (long)*f_pos / NODE_DATA_SIZE;
	int rest = (long)*f_pos % NODE_DATA_SIZE;
	
	struct node *curr = follow(dev, item);

	ssize_t retval = -ENOMEM;
	if (!curr) {
		goto out;
	}
	if (!curr->data) {
		curr->data = kmalloc(NODE_DATA_SIZE, GFP_KERNEL);
		if (!curr->data) {
			goto out;
		}
		memset(curr->data, 0, NODE_DATA_SIZE);
	}
	count = (count > NODE_DATA_SIZE - rest)? NODE_DATA_SIZE - rest : count;

	if (copy_from_user(curr->data+rest, buf, count)) {
		retval = -EFAULT;
		goto out;
	}
	*f_pos += count;
	retval = count;
	
	if (dev->size < *f_pos) {
		dev->size = *f_pos;
	}

  out:
	return retval;
}

ssize_t myread(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct haruDev *dev = filp->private_data;
	ssize_t retval = 0;
	if (*f_pos >= dev->size) {
		goto out;
	}
	if (*f_pos + count > dev->size) {
		count = dev->size - *f_pos;
	}
	int item = (long)*f_pos / NODE_DATA_SIZE;
	int rest = (long)*f_pos % NODE_DATA_SIZE;

	struct node *curr = follow(dev, item);
	if (!curr || !curr->data) {
		goto out;
	}
	count = (count > NODE_DATA_SIZE - rest)? NODE_DATA_SIZE - rest : count;
	
	if (copy_to_user(buf, curr->data+rest, count)) {
		retval = -EFAULT;
		goto out;
	}
	*f_pos += count;
	retval = count;

  out:
	return retval;
}

long myioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	if (_IOC_TYPE(cmd) != HARUDEV_IOC_MAGIC || _IOC_NR(cmd) > HARUDEV_IOC_MAXNR) {
		return -ENOTTY;
	}

	int retval = 0;
	int err = 0;
	if (_IOC_DIR(cmd) & _IOC_READ) {
		err = !access_ok((void __user *)arg, _IOC_SIZE(cmd));
	}
	else if (_IOC_DIR(cmd) & _IOC_WRITE) {
		err =  !access_ok((void __user *)arg, _IOC_SIZE(cmd));
	}
	if (err) {
		return -EFAULT;
	}

	switch(cmd) {
		case HARUDEV_IOCGSIZE:
			retval = __put_user(mydev->size, (int __user *)arg);
			break;
		default:
			retval = -ENOTTY;
	}
	return retval;
}

struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = myopen,
	.release = myrelease,
	.write = mywrite,
	.read = myread,
	.unlocked_ioctl = myioctl
};

static void setup_cdev(struct haruDev *dev)
{
	int devno = MKDEV(haruDev_major, haruDev_minor);
	cdev_init(&dev->cdev, &fops);
	dev->cdev.owner = THIS_MODULE;
	int err = cdev_add(&dev->cdev, devno, 1);

	if (err) {
		printk(KERN_NOTICE "Error %d adding haruDev", err);
	}
}

static void haruDev_cleanup_module(void)
{
	dev_t dev = MKDEV(haruDev_major, haruDev_minor);
	trim(mydev);
	if (mydev) {
		kfree(mydev);
		printk(KERN_INFO "haruDev: freed the memory.\n");
	}
	unregister_chrdev_region(dev, 1);
	printk(KERN_INFO "haruDev: unregister the device.\n");
	printk(KERN_ALERT "Goodbye world\n");
};

static int haruDev_init_module(void)
{
	printk(KERN_ALERT "Hello world\n");

	int result = 0;
	dev_t dev = 0; 
	if (haruDev_major) {
		dev = MKDEV(haruDev_major, haruDev_minor);
		result = register_chrdev_region(dev, 1, "haruDev");	// 1 -> 只有註冊一個裝置
	} else {
		result = alloc_chrdev_region(&dev, haruDev_minor, 1, "haruDev");
		haruDev_major = MAJOR(dev);
	}
	if (result < 0) {
		printk(KERN_WARNING "haruDev: can't get major %d\n", haruDev_major);
		return result;
	}

	printk(KERN_INFO "haruDev: successfully register device, major: %d, minor: %d\n", MAJOR(dev), MINOR(dev));
	mydev = kmalloc(sizeof(struct haruDev), GFP_KERNEL);
	if (!mydev) {
		result = -ENOMEM;
		goto fail;
	}
	memset(mydev, 0, sizeof(struct haruDev));
	mydev->head = NULL;
	mydev->size = 0;
	setup_cdev(mydev);
	printk("haruDev: ioctl command HARUDEV_IOCGSIZE = %lu\n", HARUDEV_IOCGSIZE);
	return 0; // succeed

  fail:
	haruDev_cleanup_module();
	return result;
};



module_init(haruDev_init_module);
module_exit(haruDev_cleanup_module);
