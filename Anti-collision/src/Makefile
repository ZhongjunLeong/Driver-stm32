KERNELDIR := /home/lzj/linux/linux-5.4.31
CURRENT_PATH := $(shell pwd)

obj-m := i2c-drive.o gpio.o
i2c-drive-objs =   rd04.o

build:	kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean