#include <linux/atomic.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/wait.h>

#include "hwbutton.h"

struct hwbutton_dev {
	struct miscdevice miscdev;
	atomic_t value;
	atomic_t click_seq;
	wait_queue_head_t wq;
};

struct hwbutton_fh {
	struct hwbutton_dev *hb;
	unsigned int last_seq;
};

static struct hwbutton_dev g_hwbutton;

static inline struct hwbutton_dev *hb_from_miscdev(struct miscdevice *m)
{
	return container_of(m, struct hwbutton_dev, miscdev);
}

static int hwbutton_open(struct inode *inode, struct file *filp)
{
	struct miscdevice *m = filp->private_data;
	struct hwbutton_fh *fh;

	fh = kzalloc(sizeof(*fh), GFP_KERNEL);
	if (!fh)
		return -ENOMEM;

	fh->hb = hb_from_miscdev(m);
	fh->last_seq = atomic_read(&fh->hb->click_seq);
	filp->private_data = fh;
	return 0;
}

static int hwbutton_release(struct inode *inode, struct file *filp)
{
	kfree(filp->private_data);
	return 0;
}

static long hwbutton_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct hwbutton_fh *fh = filp->private_data;
	struct hwbutton_dev *hb = fh->hb;

	switch (cmd) {
	case HWBUTTON_IOC_GET_VALUE: {
		__u32 v = (__u32)atomic_read(&hb->value);

		if (copy_to_user((void __user *)arg, &v, sizeof(v)))
			return -EFAULT;
		return 0;
	}
	case HWBUTTON_IOC_TRIGGER:
		pr_info("hwbutton: clicked\n");
		atomic_set(&hb->value, 0);
		atomic_inc(&hb->click_seq);
		wake_up_interruptible(&hb->wq);
		return 0;
	default:
		return -ENOTTY;
	}
}

static __poll_t hwbutton_poll(struct file *filp, poll_table *wait)
{
	struct hwbutton_fh *fh = filp->private_data;
	struct hwbutton_dev *hb = fh->hb;
	__poll_t mask = 0;

	poll_wait(filp, &hb->wq, wait);

	if ((unsigned int)atomic_read(&hb->click_seq) != fh->last_seq) {
		fh->last_seq = atomic_read(&hb->click_seq);
		mask |= EPOLLIN | EPOLLRDNORM;
	}

	return mask;
}

static const struct file_operations hwbutton_fops = {
	.owner          = THIS_MODULE,
	.open           = hwbutton_open,
	.release        = hwbutton_release,
	.unlocked_ioctl = hwbutton_ioctl,
	.compat_ioctl   = hwbutton_ioctl,
	.poll           = hwbutton_poll,
};

static ssize_t value_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct hwbutton_dev *hb = dev_get_drvdata(dev);

	return sysfs_emit(buf, "%d\n", atomic_read(&hb->value));
}
static DEVICE_ATTR_RO(value);

static ssize_t trigger_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct hwbutton_dev *hb = dev_get_drvdata(dev);
	long v;

	if (kstrtol(buf, 10, &v) || v != 1)
		return -EINVAL;

	pr_info("hwbutton: clicked\n");
	atomic_set(&hb->value, 0);
	atomic_inc(&hb->click_seq);
	wake_up_interruptible(&hb->wq);
	return count;
}
static DEVICE_ATTR_WO(trigger);

static struct attribute *hwbutton_attrs[] = {
	&dev_attr_value.attr,
	&dev_attr_trigger.attr,
	NULL,
};
ATTRIBUTE_GROUPS(hwbutton);

static int __init hwbutton_init(void)
{
	int ret;

	atomic_set(&g_hwbutton.value, 0);
	atomic_set(&g_hwbutton.click_seq, 0);
	init_waitqueue_head(&g_hwbutton.wq);

	g_hwbutton.miscdev.minor  = MISC_DYNAMIC_MINOR;
	g_hwbutton.miscdev.name   = "hwbutton";
	g_hwbutton.miscdev.fops   = &hwbutton_fops;
	g_hwbutton.miscdev.groups = hwbutton_groups;

	ret = misc_register(&g_hwbutton.miscdev);
	if (ret) {
		pr_err("hwbutton: misc_register failed: %d\n", ret);
		return ret;
	}

	dev_set_drvdata(g_hwbutton.miscdev.this_device, &g_hwbutton);
	pr_info("hwbutton: registered /dev/hwbutton\n");
	return 0;
}

static void __exit hwbutton_exit(void)
{
	misc_deregister(&g_hwbutton.miscdev);
	pr_info("hwbutton: unregistered\n");
}

module_init(hwbutton_init);
module_exit(hwbutton_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("MyOEM");
MODULE_DESCRIPTION("MyOEM simulated hardware button misc device");
