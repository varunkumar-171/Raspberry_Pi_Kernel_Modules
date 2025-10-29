#include <linux/module.h>
#include <linux/init.h>

static int __init my_init(void)
{
	printk(KERN_INFO "Hello world - kernel loaded\n");
	return 0;
}

static void __exit my_exit(void)
{
	printk(KERN_INFO "Kernel Unloading!!\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Varun Kumar");
MODULE_DESCRIPTION("A simple linux kernel module demonstrating the printk functionality");
