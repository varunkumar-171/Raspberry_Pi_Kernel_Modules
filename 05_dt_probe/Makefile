obj-m += dt_probe.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

dt:
	dtc -@ -I dts -O dtb -o my_dtoverlay.dtbo my_dtoverlay.dts

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rf my_dtoverlay.dtbo
