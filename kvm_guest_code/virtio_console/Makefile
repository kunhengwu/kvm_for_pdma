
obj-m := virtio_console.o hvc_console.o
KERNELDIR := /usr/src/kernels/3.10.0-327.22.2.el7.x86_64/

PWD := $(shell pwd)
all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean