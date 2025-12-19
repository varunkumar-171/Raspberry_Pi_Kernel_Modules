/*
 *    Simple SPI Kernel module for the SX1278.
 *    Implementation based on spidev kernel module.
 *
 *
 */


#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/property.h>
#include <linux/slab.h>
#include <linux/compat.h>

#include <linux/spi/spi.h>

#include <linux/uaccess.h>

#define SPIDEV_MAJOR			154
#define N_SPI_MINORS			1

static DECLARE_BITMAP(minors, N_SPI_MINORS);

static_assert(N_SPI_MINORS > 0 && N_SPI_MINORS <= 256);

static LIST_HEAD(device_list); 			/*Keep track of registered spi devices*/
static DEFINE_MUTEX(device_list_lock);		/*Mutex lock for device operations*/

struct my_spi_device_data{
	dev_t			devt;
	struct mutex		spi_lock;
	struct spi_device 	*my_spi_device;
	struct list_head 	device_entry;

	unsigned		users;
	struct mutex 		buf_lock;
	u8 			spi_tx_buffer[32];
	u8 			spi_rx_buffer[32];
	u32			speed_hz;
};

static unsigned bufsiz = 4096;


static struct class spidev_class = {
	.name = "my_spidev",
};


static inline ssize_t spidev_sync_read(struct spidev_data *my_spidev_data, size_t len)
{
	struct spi_transfer	t = {
			.rx_buf		= my_spidev_data->rx_buffer,
			.len		= len,
			.speed_hz	= my_spidev_data->speed_hz,
		};
	struct spi_message	m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return spidev_sync(my_spidev_data, &m);
}

static ssize_t spidev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct my_spi_device_data	*my_spidev_data;
	ssize_t			status;

	/* chipselect only toggles at start or end of operation */
	if (count > bufsiz)
		return -EMSGSIZE;

	my_spidev_data = filp->private_data;

	mutex_lock(&my_spidev_data->buf_lock);
	status = spidev_sync_read(my_spidev_data, count);				/* If success, copy to user*/
	if (status > 0) {
		unsigned long	missing;

		missing = copy_to_user(buf, my_spidev_data->spi_rx_buffer, status);
		if (missing == status)
			status = -EFAULT;
		else
			status = status - missing;
	}
	mutex_unlock(&my_spidev_data->buf_lock);

	return status;
}


static int spidev_open(struct inode *inode, struct file *filp)
{

	struct my_spi_device_data	*my_spidev_data = NULL, *iter;
	int			status = -ENXIO;

	mutex_lock(&device_list_lock);

	list_for_each_entry(iter, &device_list, device_entry) {
		if (iter->devt == inode->i_rdev) {
			status = 0;
			my_spidev_data = iter;
			break;
		}
	}

	if (!my_spidev_data) {
		pr_debug("spidev: nothing for minor %d\n", iminor(inode));
		goto err_find_dev;
	}

	if (!my_spidev_data->spi_tx_buffer) {
		my_spidev_data->spi_tx_buffer = kmalloc(bufsiz, GFP_KERNEL);
		if (!my_spidev_data->spi_tx_buffer) {
			status = -ENOMEM;
			goto err_find_dev;
		}
	}

	if (!my_spidev_data->spi_rx_buffer) {
		my_spidev_data->spi_rx_buffer = kmalloc(bufsiz, GFP_KERNEL);
		if (!my_spidev_data->spi_rx_buffer) {
			status = -ENOMEM;
			goto err_alloc_rx_buf;
		}
	}

	my_spidev_data->users++;
	filp->private_data = my_spidev_data;
	stream_open(inode, filp);

	mutex_unlock(&device_list_lock);
	return 0;

	err_alloc_rx_buf:
		kfree(my_spidev_data->tx_buffer);
		my_spidev_data->tx_buffer = NULL;
	err_find_dev:
		mutex_unlock(&device_list_lock);
	return status;

	return status;
}

static int spidev_release(struct inode *inode, struct file *filp)
{
	struct my_spi_device_data	*my_spidev_data;
	int			dofree;

	mutex_lock(&device_list_lock);
	my_spidev_data = filp->private_data;
	filp->private_data = NULL;

	mutex_lock(&spidev->spi_lock);
	dofree = (spidev->spi == NULL);
	mutex_unlock(&spidev->spi_lock);

	/* last close? */
	my_spidev_data->users--;
	if (!my_spidev_data->users) {			/* Check if no one is using any of the spi devices */

		kfree(my_spidev_data->spi_tx_buffer);
		my_spidev_data->spi_tx_buffer = NULL;

		kfree(my_spidev_data->spi_rx_buffer);
		my_spidev_data->spi_rx_buffer = NULL;

		if (dofree)
			kfree(my_spidev_data);
		else
			my_spidev_data->speed_hz = my_spidev_data->spi->max_speed_hz;
	}

	mutex_unlock(&device_list_lock);

	return 0;
}


spidev_sync_write(struct my_spi_device_data *spidev, size_t len)
{
    struct spi_transfer t = {
        .tx_buf         = spidev->buffer,			/* send buffer */
        .len            = len,					/* send data length */
    };
    struct spi_message      m;
    spi_message_init(&m);					/* initialization spi_message */
    spi_message_add_tail(&t, &m);				/* add new spi_transfer to the tail of spi_message queue */
    return spidev_sync(spidev, &m);				/* synchronized reading and writing */
}

static ssize_t spidev_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    struct my_spi_device_data      *my_spidev_data;
    ssize_t                 status = 0;
    unsigned long           missing;
    if (count > bufsiz)
            return -EMSGSIZE;
    my_spidev_data = filp->private_data;
    mutex_lock(&my_spidev_data->buf_lock);
    missing = copy_from_user(my_spidev_data->spi_tx_buffer, buf, count);	/* transfer the data from user space to kernel space */
    if (missing == 0) {
            status = spidev_sync_write(my_spidev_data, count);			/* call write synchronization function */
    } else
            status = -EFAULT;
    mutex_unlock(&my_spidev_data->buf_lock);
    return status;
}

static const struct file_operations spidev_fops = {
	.owner =	THIS_MODULE,
	.write =	spidev_write,
	.read =		spidev_read,
	//.unlocked_ioctl = spidev_ioctl,
	//.compat_ioctl = spidev_compat_ioctl,
	.open =		spidev_open,
	.release =	spidev_release,
};

static const struct of_device_id my_driver_ids[] = {
	{.compatible = "semtech,sx1278"	},
	{ /*Sentinel*/},
};
MODULE_DEVICE_TABLE(of, my_driver_ids);

static const struct spi_device_id sx1278_id[] = {
    { "semtech,sx1278", 0 },
    { /* Sentinel */ },
};
MODULE_DEVICE_TABLE(spi, sx1278_id);


static int my_spi_device_probe(struct spi_device *spi_client){

	struct my_spi_device_data *my_spidev_data;
	int status;
	unsigned long minor;

	pr_info("Probing SPI device!!\n");
	pr_info("SPI device modalias: %s\n", spi_client->modalias);
	pr_info("SPI max speed hz: %d\n", spi_client->max_speed_hz);
	pr_info("SPI bits per word: %d\n", spi_client->bits_per_word);

	my_spidev_data = kzalloc(sizeof(*my_spidev_data), GFP_KERNEL);
	if(!my_spidev_data){
		pr_err("Unable to allocate memory for device!!\n");
		return -ENOMEM;
	}

	my_spidev_data->my_spi_device = spi_client;
	mutex_init(&my_spidev_data->spi_lock);
	mutex_init(&my_spidev_data->buf_lock);

	INIT_LIST_HEAD(&my_spidev_data->device_entry);


	mutex_lock(&device_list_lock);
	minor = find_first_zero_bit(minors, N_SPI_MINORS);			/* Find the first free minor number of the major */
	if (minor < N_SPI_MINORS) {
		struct device *dev;

		my_spidev_data->devt = MKDEV(SPIDEV_MAJOR, minor);
		dev = device_create(&spidev_class, &spi_client->dev, my_spidev_data->devt,
				    my_spidev_data, "my_spidev");
		status = PTR_ERR_OR_ZERO(dev);
	} else {
		dev_dbg(&spi_client->dev, "no minor number available!\n");
		status = -ENODEV;
	}
	if(status == 0){
		set_bit(minor, minors);						/* Cannot allocate the minor number anymore */
		list_add(&my_spidev_data->device_entry, &device_list);		/* Add the device to the list of spi devices */
	}
	mutex_unlock(&device_list_lock);

	my_spidev_data->speed_hz = spi_client->max_speed_hz;

	if (status == 0)
		spi_set_drvdata(spi_client, my_spidev_data);
	else
		kfree(my_spidev_data);

	return status;
}

static void my_spi_device_remove(struct spi_device *spi_client){
	struct my_spi_device_data *my_spidev_data = spi_get_drvdata(spi_client);

	if(!my_spidev_data){
		pr_err("Driver does not exist\n");
		return;
	}

	mutex_lock(&device_list_lock);

	mutex_lock(&my_spidev_data->spi_lock);
	my_spidev_data->my_spi_device = NULL;
	mutex_unlock(&my_spidev_data->spi_lock);

	list_del(&my_spidev_data->device_entry);
	device_destroy(&spidev_class, my_spidev_data->devt);
	clear_bit(MINOR(my_spidev_data->devt), minors);				/* Make the minor number free again */

	if (my_spidev_data->users == 0){
		kfree(my_spidev_data);
	}
	mutex_unlock(&device_list_lock);
	pr_info("SPI Driver removed\n");
}

static struct spi_driver my_spi_driver = {
	.probe = my_spi_device_probe,
	.remove = my_spi_device_remove,
	.id_table = sx1278_id,
	.driver = {
		.owner = THIS_MODULE,
		.name  = "my_sx1278",
		.of_match_table = my_driver_ids,
	},
};

static int __init my_spidev_init(void)
{
	int status;

	status = register_chrdev(SPIDEV_MAJOR, "my_spidev", &spidev_fops);
	if (status < 0)
		return status;

	status = class_register(&spidev_class);
	if (status) {
		unregister_chrdev(SPIDEV_MAJOR, my_spi_driver.driver.name);
		return status;
	}

	status = spi_register_driver(&my_spi_driver);
	if (status < 0) {
		class_unregister(&spidev_class);
		unregister_chrdev(SPIDEV_MAJOR, my_spi_driver.driver.name);
	}
	return status;
}
module_init(my_spidev_init);

static void __exit my_spidev_exit(void)
{
	spi_unregister_driver(&my_spi_driver);
	class_unregister(&spidev_class);
	unregister_chrdev(SPIDEV_MAJOR, my_spi_driver.driver.name);
}
module_exit(my_spidev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Varun Kumar");
MODULE_DESCRIPTION("A simple SPI linux kernel module to access the sx1278 lora module");
