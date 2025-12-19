obj-m += i2c_ssd1306.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

dt:
	dtc -@ -I dts -O dtb -o ssd1306_overlay.dtbo ssd1306_overlay.dts

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rf ssd1306_overlay.dtbo
