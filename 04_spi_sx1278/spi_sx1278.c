#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/spi/spi.h>

#define SPI_BUS 0

static struct spi_device *sx1278_device

static int __init my_init(void)
{
	return 0;
}

static void __exit my_exit(void)
{
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Varun Kumar");
MODULE_DESCRIPTION("A simple linux kernel module to access the sx1278 lora module");
