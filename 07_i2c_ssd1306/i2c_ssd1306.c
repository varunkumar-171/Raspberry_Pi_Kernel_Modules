/*
 *    Simple i2C Kernel module for the SSD1306
 *
 *
 */
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>

#include "ssd1306.h"

#define MY_CDEV_MAJOR 69
#define MY_CDEV_MINOR 0
#define MAX_CDEV_MINORS 1

static struct class *ssd1306_class = NULL;

struct ssd1306_data {
    struct i2c_client *client;
    struct cdev cdev;
    dev_t dev_num;
    unsigned char frame_buffer[1024 + 1];	// Command + frame
};

struct ssd1306_data *my_ssd1306_data;

#define BUFFER_SIZE sizeof(my_ssd1306_data->frame_buffer)

int i2c_client_open(struct inode * inode, struct file * file);
int i2c_client_release(struct inode * inode, struct file * file);
static ssize_t i2c_client_write(struct file * file, const char * buffer, size_t count, loff_t * offset);
static ssize_t i2c_client_read(struct file * file, char __user * buffer, size_t count, loff_t * offset);

static int my_i2c_device_probe(struct i2c_client *i2c_client, const struct i2c_device_id * id);
static void my_i2c_device_remove(struct i2c_client *i2c_client);

int ssd1306_controller_init(void);
int ssd1306_set_cursor_at_start(void);

static const struct file_operations i2c_fops = {
	.owner =	THIS_MODULE,
	.write =	i2c_client_write,
	.read =		i2c_client_read,
	//.unlocked_ioctl = spidev_ioctl,
	//.compat_ioctl = spidev_compat_ioctl,
	.open =		i2c_client_open,
	.release =	i2c_client_release,
};

static const struct of_device_id my_driver_ids[] = {
	{.compatible = "solomon,ssd1306", },
	{ /*Sentinel*/},
};
MODULE_DEVICE_TABLE(of, my_driver_ids);

static struct i2c_device_id driver_device_id[] = {{"solomon,ssd1306", 0},
                                                  {/*sentinel*/}};

MODULE_DEVICE_TABLE(i2c, driver_device_id);

int i2c_client_open(struct inode * inode, struct file * file){
	return 0;
}

int i2c_client_release(struct inode * inode, struct file * file){
	return 0;
}

/*
 * Function: send command byte
 * ----------------------------
 *   Sends a command to the i2c client
 *   D/C# bit is set to logic “0”
 *   command: the command
 *
 *   returns: Returns negative errno, or else the number of bytes written.
 */
int sendCommand(unsigned char command){
 	unsigned char buf[2];
    	buf[0] = 0x00;
    	buf[1] = command;
    	return i2c_master_send(my_ssd1306_data->client, buf, 2);
}

/*
 * Function: send data byte
 * ----------------------------
 *   Sends a data byte to the i2c client
 *
 *   data: the data to be written
 *   D/C# bit is set to logic “1”
 *   returns: Returns negative errno, or else the number of bytes written.
 */
int sendDataByte(unsigned char data){
    	unsigned char buf[2];
    	buf[0] = 0x40;
    	buf[1] = data;
    	return i2c_master_send(my_ssd1306_data->client, buf, 2);
}

/*
 * Function: send data block
 * ----------------------------
 *   Sends a block of data bytes to the i2c client
 *
 *   block: the data to be written
 *   size: size of the data block
 *
 *   returns: Returns negative errno, or else the number of bytes written.
 */
int sendDataBlock(unsigned char *block, const unsigned int size){
	int ret;
    	ret = i2c_master_send(my_ssd1306_data->client, block, size);
    	return ret;
}

int ssd1306_controller_init(void)
{
	sendCommand(SSD1306_DISPLAY_OFF);
    	sendCommand(SSD1306_SET_CLK_DR_OSC_FRQ);
    	sendCommand(CLK_DR_OSC_FRQ);
    	sendCommand(SSD1306_SET_MUX_RATIO);
    	sendCommand(MUX64);
    	sendCommand(SSD1306_SET_DISPLAY_OFFSET);
    	sendCommand(OFFSET_VALUE);
    	sendCommand(SSD1306_SET_DISPLAY_START_FIRST);
    	sendCommand(SSD1306_SET_CHARGE_PUMP);
    	sendCommand(ENABLE_PUMP);
    	sendCommand(SSD1306_SET_MEM_ADDR_MODE);
    	sendCommand(ADDR_MODE_HZ);
    	sendCommand(SSD1306_SET_SEGMENT_REMAP127);
    	sendCommand(SSD1306_SET_COM_OUT_DESCENDING);
    	sendCommand(SSD1306_SET_COM_CONFIG);
    	sendCommand(COM_CONFIG);
    	sendCommand(SSD1306_SET_CONTRAST);
    	sendCommand(CONTRAST);
    	sendCommand(SSD1306_SET_PRE_CHARGE_PERIOD);
    	sendCommand(PERIOD);
    	sendCommand(SSD1306_SET_VCOMH_DESELECT_LEVEL);
    	sendCommand(VCOMH_DESELECT_LEVEL);
    	sendCommand(SSD1306_SET_DISPLAY_ON_RAM);
    	sendCommand(SSD1306_SET_DISPLAY_NORMAL);
    	sendCommand(SSD1306_SET_SCROLL_OFF);
    	sendCommand(SSD1306_DISPLAY_ON);

    	pr_info("SSD1306 display initialized!!!\n");

	memcpy(my_ssd1306_data->frame_buffer + 1, LOGO_BITMAP, SCREEN_PIXELS);	/* Command offset at [0]*/
    	sendDataBlock(my_ssd1306_data->frame_buffer, SCREEN_PIXELS);		/* Clear any garbage pixels */

	return 0;
}

int ssd1306_set_cursor_at_start(void){
	sendCommand(SSD1306_SET_COLUMN);
    	sendCommand(DISPLAY_BEGIN_COL);
    	sendCommand(DISPLAY_END_COL);
    	sendCommand(SSD1306_SET_PAGE);
    	sendCommand(DISPLAY_BEGIN_PAGE);
    	sendCommand(DISPLAY_END_PAGE);

	return 0;
}

int ssd1306_clear_screen(void){
	ssd1306_set_cursor_at_start();
	memset(my_ssd1306_data->frame_buffer + 1, 0x00, BUFFER_SIZE);		/* Command offset at [0]*/
    	sendDataBlock(my_ssd1306_data->frame_buffer, SCREEN_PIXELS);

	return 0;
}

static ssize_t i2c_client_read(struct file * file, char __user * buffer, size_t count, loff_t * offset){
	return 0;
}

static ssize_t i2c_client_write(struct file * file, const char * buffer, size_t count, loff_t * offset){
	size_t bytes_to_write = (count + *offset > BUFFER_SIZE) ? BUFFER_SIZE : (count - *offset);
	size_t bytes_not_written = 0, bytes_written = 0;

	if(*offset > BUFFER_SIZE){
		printk(KERN_ERR "Cannot write to file. EOF\n");
		return 0;
	}


	bytes_not_written = copy_from_user(my_ssd1306_data->frame_buffer + 1, buffer, bytes_to_write);		/* Overwrite only frame */

	bytes_written = bytes_to_write - bytes_not_written;
	if(bytes_not_written){
		printk(KERN_ERR "Read from file failed!! Could only write %zd bytes\n", bytes_written);
	}

	// *offset += bytes_written;

	if(my_ssd1306_data->frame_buffer[1] == 0x00){
		// pr_info("Received command %x\n", my_ssd1306_data->frame_buffer[1]);
		sendCommand(my_ssd1306_data->frame_buffer[1]);
	}
	else if(my_ssd1306_data->frame_buffer[1] == 0x69){
		ssd1306_clear_screen();
	}
	else if(my_ssd1306_data->frame_buffer[1] == 0x68){
		ssd1306_set_cursor_at_start();
	}
	else{
		sendDataBlock(my_ssd1306_data->frame_buffer, BUFFER_SIZE);
	}

	return bytes_written;
}

static int ssd1306_uevent(struct device *dev, struct kobj_uevent_env *env)
{
    add_uevent_var(env, "DEVMODE=%#o", 0666);
    return 0;
}


static struct i2c_driver my_i2c_driver = {
	.probe = my_i2c_device_probe,
	.remove = my_i2c_device_remove,
	.id_table = driver_device_id,
	.driver = {
		.owner = THIS_MODULE,
		.name  = "my_ssd1306",
		.of_match_table = my_driver_ids,
	},
};

static int my_i2c_device_probe(struct i2c_client *i2c_client, const struct i2c_device_id * id)
{
	int status;

	pr_info("Probing i2c device!!\n");

	if(i2c_client->addr != 0x3c){
		pr_err("Wrong i2c address\n");
		return -1;
	}

	my_ssd1306_data->client = i2c_client;
	my_ssd1306_data->frame_buffer[0] = 0x40;

	ssd1306_controller_init();

	return status;
}

static void my_i2c_device_remove(struct i2c_client *i2c_client){
}

static int __init my_ssd1306_init(void)
{
	int status;

	my_ssd1306_data = kzalloc(sizeof(*my_ssd1306_data), GFP_KERNEL);
	if(!my_ssd1306_data){
		pr_err("Unable to allocate memory!!\n");
		return -ENOMEM;
	}

	status = alloc_chrdev_region(&my_ssd1306_data->dev_num, MY_CDEV_MAJOR, MAX_CDEV_MINORS, "my_ssd1306");
	if(status < 0){
		printk(KERN_ERR "Unable to allocate character device\n");
		return status;
	}

	ssd1306_class = class_create(THIS_MODULE, "my_ssd1306");
	ssd1306_class->dev_uevent = ssd1306_uevent;

	my_ssd1306_data->dev_num = MKDEV(MY_CDEV_MAJOR, MY_CDEV_MINOR);
	cdev_init(&my_ssd1306_data->cdev, &i2c_fops);
	my_ssd1306_data->cdev.owner = THIS_MODULE;

	cdev_add(&my_ssd1306_data->cdev, my_ssd1306_data->dev_num, 1);
	device_create(ssd1306_class, NULL, my_ssd1306_data->dev_num, NULL, "my_ssd1306-%d", 1);
	printk(KERN_INFO "Created a character device for i2c client with major number %d and minor numer %d\n", MAJOR(my_ssd1306_data->dev_num), MINOR(my_ssd1306_data->dev_num));

	status = i2c_add_driver(&my_i2c_driver);
	if(status < 0){
		pr_err("Unable to add driver for i2c device!!\n");
		return -1;
	}
	else{
		pr_info("Driver added for SSD1306 oled display\n");
		status = 0;
	}

	return status;
}
module_init(my_ssd1306_init);

static void __exit my_ssd1306_exit(void)
{
	ssd1306_clear_screen();
	cdev_del(&my_ssd1306_data->cdev);
	device_destroy(ssd1306_class, my_ssd1306_data->dev_num);
	class_destroy(ssd1306_class);
	unregister_chrdev_region(MKDEV(MY_CDEV_MAJOR, 0), MAX_CDEV_MINORS);

	i2c_del_driver(&my_i2c_driver);

	pr_info("Removed ssd1306 i2c driver\n");

	kfree(my_ssd1306_data);
}
module_exit(my_ssd1306_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Varun Kumar");
MODULE_DESCRIPTION("A simple i2c linux kernel module to access the SSD1306 oled module");
