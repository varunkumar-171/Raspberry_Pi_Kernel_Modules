#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/errno.h>

#define BUFFER_SIZE 128

static struct proc_dir_entry * procfs_folder;
static struct proc_dir_entry * procfs_file;

static uint8_t dummy_buffer[BUFFER_SIZE];

static ssize_t procfs_read(struct file *file, char __user *buf, size_t count, loff_t *offset);
static ssize_t procfs_write(struct file *file, const char __user *buf, size_t count, loff_t *offset);

static struct proc_ops fops = {
	.proc_read = procfs_read,
	.proc_write = procfs_write,
};

static ssize_t procfs_read(struct file * file, char __user * buffer, size_t count, loff_t * offset){

	size_t bytes_to_copy = (count + *offset < BUFFER_SIZE) ? count : (BUFFER_SIZE - *offset);
	size_t bytes_not_copied = 0, bytes_copied = 0;

	printk(KERN_INFO "Requested %zd bytes, Reading %zd bytes with offset %lld\n", count, bytes_to_copy, *offset);

	if(*offset > BUFFER_SIZE){
		printk(KERN_ERR "Cannot read data. EOF\n");
		return 0;
	}

	bytes_not_copied = copy_to_user(buffer, &dummy_buffer[*offset], bytes_to_copy);
	bytes_copied = bytes_to_copy - bytes_not_copied;
	if(bytes_not_copied){
		printk(KERN_ERR "Read from file failed!! Could only copy %zd bytes\n", bytes_copied);
	}

	*offset += bytes_copied;
	printk(KERN_INFO "Successfully read %zd bytes\n", bytes_copied);
	return bytes_copied;
}

static ssize_t procfs_write(struct file * file, const char * buffer, size_t count, loff_t * offset){

	size_t bytes_to_write = (count + *offset > BUFFER_SIZE) ? BUFFER_SIZE : (count - *offset);
	size_t bytes_not_written = 0, bytes_written = 0;

	printk(KERN_INFO "Requested %zd bytes, Writing %zd bytes with offset %lld\n", count, bytes_to_write, *offset);

	if(*offset > BUFFER_SIZE){
		printk(KERN_ERR "Cannot write to file. EOF\n");
		return 0;
	}

	bytes_not_written = copy_from_user(&dummy_buffer[*offset], buffer, bytes_to_write);
	bytes_written = bytes_to_write - bytes_not_written;
	if(bytes_not_written){
		printk(KERN_ERR "Read from file failed!! Could only write %zd bytes\n", bytes_written);
	}

	*offset += bytes_written;
	printk(KERN_INFO "Successfully wrote %zd bytes\n", bytes_written);
	return bytes_written;
}


static int __init my_init(void)
{
	procfs_folder = proc_mkdir("my_procfs_folder", NULL);
	if(procfs_folder == NULL){
		pr_err("Error creating procfs sample folder. -(ENONMEM)\n");
		return -ENOMEM;
	}

	procfs_file = proc_create("my_procfs_file", 0666, procfs_folder, &fops);
	if(procfs_file == NULL){
		pr_err("Error creating procfs sample file. -(ENONMEM)\n");
		proc_remove(procfs_folder);
		return -ENOMEM;
	}

	pr_info("Created /proc/my_procfs_folder/my_procfs_file\n");

	return 0;
}

static void __exit my_exit(void)
{
	proc_remove(procfs_file);
	proc_remove(procfs_folder);
	pr_info("Removed /proc/my_procfs_folder/my_procfs_file\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Varun Kumar");
MODULE_DESCRIPTION("A simple linux kernel module for procfs");
