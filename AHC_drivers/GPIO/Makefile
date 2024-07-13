KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

# module_RDC_gpio_platform setting

obj-m += gpio-ahc0b10.o

all:
# use Tab
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	#
	modinfo gpio-ahc0b10.ko
	#
	
clean:
# use Tab
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	
install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
	# install to avoid module symbols error
	# ignore SSL error
	
check:
	$(KDIR)/scripts/checkpatch.pl -no-tree -f gpio-ahc0b10.c