
ifneq ($(KERNELRELEASE),)

obj-m := main.o
ccflags-y := -std=gnu99 -Wno-declaration-after-statement -g

else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)


defaults:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

endif

app:
	gcc app.c -o app
clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions app
