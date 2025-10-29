#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mod_devicetable.h>
#include <linux/property.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>

static int dt_probe(struct platform_device* pdev);
static int dt_remove(struct platform_device* pdev);

static const struct of_device_id my_driver_ids[] = {
	{
		.compatible = "semtech,mydev",
	},
	{ }
};

MODULE_DEVICE_TABLE(of, my_driver_ids);

static struct platform_driver my_driver = {
	.probe = dt_probe,
	.remove = dt_remove,
	.driver = {
		.name = "my_device_driver",
		.of_match_table = my_driver_ids,
	},
};

static int dt_probe(struct platform_device * pdev){
	struct device * dev = &pdev->dev;
	const char* label;
	int my_value, ret;

	/* Check if the properties exist*/
	if(!device_property_present(dev, "label")){
		pr_err("dt_probe error. Device property 'label' not found ! \n");
		return -1;
	}

	if(!device_property_present(dev, "value")){
		pr_err("dt_probe error. Device property 'value' not found ! \n");
		return -1;
	}

	/*Read the values*/
	ret = device_property_read_string(dev, "label", &label);
	if(ret){
		pr_warn("Could not read the property 'label' \n");
		return -1;
	}

	ret = device_property_read_u32(dev, "value", &my_value);
	if(ret){
		pr_warn("Could not read the property 'value'\n ");
		return -1;
	}

	pr_info("Label - %s\n", label);
	pr_info("Value - %d\n", my_value);

	return 0;
}

static int dt_remove(struct platform_device * pdev){
	return 0;
}

static int __init my_init(void)
{
	pr_info("Loading the kernel driver....\n");
	return platform_driver_register(&my_driver);    // better to use module_platform_driver(&my_driver)
}

static void __exit my_exit(void)
{
	platform_driver_unregister(&my_driver);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Varun Kumar");
MODULE_DESCRIPTION("A simple linux kernel module to access the sx1278 lora module");
