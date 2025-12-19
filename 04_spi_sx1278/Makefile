obj-m += spi_sx1278.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

dt:
	dtc -@ -I dts -O dtb -o sx1278_overlay.dtbo sx1278_overlay.dts

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rf sx1278_overlay.dtbo
