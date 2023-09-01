KERNELDIR := /home/lzj/linux/linux-5.4.31
CURRENT_PATH := $(shell pwd)

CC = arm-none-linux-gnueabihf-gcc
CFLAGS += -pthread    
LDFLAGS += -pthread   

obj-m := i2c-drive.o gpio.o
i2c-drive-objs =   rd04.o

build:	kernel_modules main

main: main.o
	$(CC) -o $@ $^ $(LDFLAGS)

main.o: main.c
	$(CC) $(CFLAGS) -c -o $@ $<

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	rm -f main main.o