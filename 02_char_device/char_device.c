#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/fs.h>

#define MY_CDEV_MAJOR 69
#define MY_CDEV_MINOR 0
#define MAX_CDEV_MINORS 2

#define BUFFER_SIZE 128

struct cdev_device_data{
	struct cdev cdev;
};

static uint8_t chdev_buffer[BUFFER_SIZE];

static char* secret_key = "Character Device Driver";
module_param(secret_key, charp, 0644);
MODULE_PARM_DESC(secret_key, "Module Parameter Demonstration");

struct cdev_device_data my_cdev_data[MAX_CDEV_MINORS];
static struct class *my_cdev_class = NULL;

static int chrdev_open(struct inode *inode, struct file *file);
static int chrdev_release(struct inode *inode, struct file *file);
static long chrdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static ssize_t chrdev_read(struct file *file, char __user *buf, size_t count, loff_t *offset);
static ssize_t chrdev_write(struct file *file, const char __user *buf, size_t count, loff_t *offset);

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.read = chrdev_read,
	.write = chrdev_write,
	.open = chrdev_open,
	.release = chrdev_release
};

static ssize_t chrdev_read(struct file * file, char __user * buffer, size_t count, loff_t * offset){

	size_t bytes_to_copy = (count + *offset < BUFFER_SIZE) ? count : (BUFFER_SIZE - *offset);
	size_t bytes_not_copied = 0, bytes_copied = 0;

	printk("Reading device: %d\n", MINOR(file->f_path.dentry->d_inode->i_rdev));
	printk(KERN_INFO "Requested %zd bytes, Reading %zd bytes with offset %lld\n", count, bytes_to_copy, *offset);

	if(*offset > BUFFER_SIZE){
		printk(KERN_ERR "Cannot read data. EOF\n");
		return 0;
	}

	bytes_not_copied = copy_to_user(buffer, &chdev_buffer[*offset], bytes_to_copy);
	bytes_copied = bytes_to_copy - bytes_not_copied;
	if(bytes_not_copied){
		printk(KERN_ERR "Read from file failed!! Could only copy %zd bytes\n", bytes_copied);
	}

	*offset += bytes_copied;
	printk(KERN_INFO "Successfully read %zd bytes\n", bytes_copied);
	return bytes_copied;
}

static ssize_t chrdev_write(struct file * file, const char * buffer, size_t count, loff_t * offset){

	size_t bytes_to_write = (count + *offset > BUFFER_SIZE) ? BUFFER_SIZE : (count - *offset);
	size_t bytes_not_written = 0, bytes_written = 0;

	printk(KERN_INFO "Writing device: %d\n", MINOR(file->f_path.dentry->d_inode->i_rdev));
	printk(KERN_INFO "Requested %zd bytes, Writing %zd bytes with offset %lld\n", count, bytes_to_write, *offset);

	if(*offset > BUFFER_SIZE){
		printk(KERN_ERR "Cannot write to file. EOF\n");
		return 0;
	}

	bytes_not_written = copy_from_user(&chdev_buffer[*offset], buffer, bytes_to_write);
	bytes_written = bytes_to_write - bytes_not_written;
	if(bytes_not_written){
		printk(KERN_ERR "Read from file failed!! Could only write %zd bytes\n", bytes_written);
	}

	*offset += bytes_written;
	printk(KERN_INFO "Successfully wrote %zd bytes\n", bytes_written);
	return bytes_written;
}

int chrdev_open(struct inode * inode, struct file * file){
	printk(KERN_INFO "My character device: open file operation\n");
	return 0;
}

int chrdev_release(struct inode * inode, struct file * file){
	printk(KERN_INFO "My character device: released file operation\n");
	return 0;
}

static int mychardev_uevent(struct device *dev, struct kobj_uevent_env *env)
{
    add_uevent_var(env, "DEVMODE=%#o", 0666);
    return 0;
}

static int __init my_init(void)
{
	int status;
	dev_t dev;

	printk(KERN_INFO "The module parameter is - \"%s\" \n", secret_key);

	status = alloc_chrdev_region(&dev, MY_CDEV_MAJOR, MAX_CDEV_MINORS, "my_cdev");
	if(status < 0){
		printk(KERN_ERR "Unable to allocate character device\n");
		return status;
	}

	my_cdev_class = class_create(THIS_MODULE, "my_cdev");
	my_cdev_class->dev_uevent = mychardev_uevent;

	for(int i = 0; i < MAX_CDEV_MINORS; i++){
		dev = MKDEV(MY_CDEV_MAJOR, i);
		cdev_init(&my_cdev_data[i].cdev, &fops);
		my_cdev_data[i].cdev.owner = THIS_MODULE;

		cdev_add(&my_cdev_data[i].cdev, dev, 1);
		device_create(my_cdev_class, NULL, dev, NULL, "my_cdev-%d", i);
		printk(KERN_INFO "Created a character device with major number %d and minor numer %d\n", MAJOR(dev), MINOR(dev));
	}

	return 0;
}

static void __exit my_exit(void)
{
	printk(KERN_INFO "Kernel Unloading!!\n");
	for(int i = 0; i < MAX_CDEV_MINORS; i++){
		printk(KERN_INFO "Deleted character device with major number %d and minor number %d\n", MY_CDEV_MAJOR, i);
		device_destroy(my_cdev_class, MKDEV(MY_CDEV_MAJOR, i));
	}

	class_unregister(my_cdev_class);
	class_destroy(my_cdev_class);

	unregister_chrdev_region(MKDEV(MY_CDEV_MAJOR, 0), MINORMASK);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Varun Kumar");
MODULE_DESCRIPTION("A simple linux kernel module for a character device");
