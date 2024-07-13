KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

# module_RDC_I2C_platform setting
#i2c-ahc0510-objs := i2c-ahc0510.o

obj-m += smb-ahc0511.o

all:
# use Tab
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	#
	modinfo smb-ahc0511.ko
	#

	
	
	
clean:
# use Tab
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	
install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
	# install to avoid module symbols error
	# ignore SSL error
	
check:
	$(KDIR)/scripts/checkpatch.pl -no-tree -f smb-ahc0511.c