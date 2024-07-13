// SPDX-License-Identifier: GPL-2.0
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the version 2 of the GNU General Public License
 * as published by the Free Software Foundation
 */
/* kernel module */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h> /* kmalloc, kzalloc */
#include <linux/kdev_t.h> /* MAJOR, MINOR */

/* interrupt & spinlock & event & thread */
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/completion.h>
#include <linux/wait.h>
#include <linux/kthread.h>
/* #include <linux/freezer.h> */

/* time */
#include <linux/time.h>
#include <linux/jiffies.h>
#include <linux/delay.h>

/* pci & pnp & platform */
/*#include <linux/pci.h> define PCI_DEVFN, PCI_SLOT, PCI_FUNC */
#include <linux/pnp.h>
#include <linux/acpi.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <asm/io.h>
#include <asm/uaccess.h>
/*#include <linux/dma-mapping.h>*/
/*#define  DMA_ADDR_INVALID (~(dma_addr_t)0)*/

/* device interface */
#include <linux/gpio/driver.h>
#include <linux/gpio/machine.h>
#include <linux/version.h>

/* our device header */
/*
 * return -ERRNO -EFAULT -ENOMEM -EINVAL
 * device -EBUSY, -ENODEV, -ENXIO
 * parameter -EOPNOTSUPP
 * pr_info pr_warn pr_err
 */
#define DRIVER_NAME		"RDC_GPIO"
#define DRIVER_VERSION		"5.0" /* based on linux version */
#define pr_fmt(fmt) DRIVER_NAME	": " fmt
#define DrvDbgShallNotRunToHere \
	pr_debug("the program shall not run to here. %s %d\n",\
	__FILE__, __LINE__)

/*#define TRUE	1*/
/*#define FALSE	0*/
#define IN
#define OUT

#define _setdatafield(data, fielddefine, value) \
	(data = (data & ~fielddefine) | \
	((value & fielddefine##_RANGE) << fielddefine##_SHIFT))

#define _getdatafield(data, fielddefine, variable) \
	(variable = ((data) & fielddefine) >> fielddefine##_SHIFT)

#define is_real_interrupt(irq) ((irq) != 0)

#define LZ_GPIOREGOFFSET_PORTDATA0		0x00
#define LZ_GPIOREGOFFSET_PORTDATA1		0x01
#define LZ_GPIOREGOFFSET_PORTDIRECTION0		0x04
#define LZ_GPIOREGOFFSET_PORTDIRECTION1		0x05
#define LZ_GPIOREGOFFSET_INTERRUPTSTATUS0	0x08
#define LZ_GPIOREGOFFSET_INTERRUPTSTATUS1	0x09
#define LZ_GPIOREGOFFSET_INTCONTROL0		0x0C
#define LZ_GPIOREGOFFSET_INTCONTROL1		0x10

#define STATUS_INTERRUPT_MASK			0xFF

#define INTCONTROL_MASK				0x000000FF
#define INTCONTROL_TRIGGERLEVEL			0x0000FF00
#define INTCONTROL_KEEPPERIOD			0x00700000
#define INTCONTROL_ENABLECONTROL		0x00800000
#define INTCONTROL_REPEATTRIGGER		0xFF000000

#define INTCONTROL_MASK_SHIFT			(0)
#define INTCONTROL_TRIGGERLEVEL_SHIFT		(8)
#define INTCONTROL_KEEPPERIOD_SHIFT		(20)
#define INTCONTROL_ENABLECONTROL_SHIFT		(23)
#define INTCONTROL_REPEATTRIGGER_SHIFT		(24)

#define INTCONTROL_MASK_RANGE			0x000000FF
#define INTCONTROL_TRIGGERLEVEL_RANGE		0x000000FF
#define INTCONTROL_KEEPPERIOD_RANGE		0x00000007
#define INTCONTROL_ENABLECONTROL_RANGE		0x00000001
#define INTCONTROL_REPEATTRIGGER_RANGE		0x000000FF

/*
 * ----
 * layer hardware driver
 */
/* lz_ layer zero */
static u8 lz_read_reg8(void __iomem *reg_base, u32 reg)
{
	/* ioread8, ioread16, ioread32 */
	/* inb, inw, inl */
	return inb((unsigned long)reg_base + reg);
}

static void lz_write_reg8(void __iomem *reg_base, u32 reg, u8 val)
{
	/* iowrite8, iowrite16, iowrite32 */
	/* outb, outw, outl */
	outb(val, (unsigned long)reg_base + reg);
}

static void lz_write_reg16(void __iomem *reg_base, u32 reg, u16 val)
{
	/* iowrite8, iowrite16, iowrite32 */
	/* outb, outw, outl */
	outw(val, (unsigned long)reg_base + reg);
}

static void lz_write_reg32(void __iomem *reg_base, u32 reg, u32 val)
{
	/* iowrite8, iowrite16, iowrite32 */
	/* outb, outw, outl */
	outl(val, (unsigned long)reg_base + reg);
}

#define PINDIRECTION_INPUT	0
#define PINDIRECTION_OUTPUT	1

#define INTERRUPTMASK_DISABLE	0
#define INTERRUPTMASK_ENABLE	1

#define INTERRUPTTRIGGERLEVEL_HIGH	0
#define INTERRUPTTRIGGERLEVEL_LOW	1

#define INTERRUPTKEEPPERIOD_2MS		0
#define INTERRUPTKEEPPERIOD_5MS		1
#define INTERRUPTKEEPPERIOD_10MS	2
#define INTERRUPTKEEPPERIOD_20MS	3
#define INTERRUPTKEEPPERIOD_40MS	4
#define INTERRUPTKEEPPERIOD_60MS	5
#define INTERRUPTKEEPPERIOD_80MS	6
#define INTERRUPTKEEPPERIOD_100MS	7

struct gpioglobalconfigure {
	u32	direction;
	u32	enablecontrol;
	u32	mask; /* default: 0 */
	u32	triggerlevel;
	u32	keepperiod;
	u32	repeattrigger; /* default: 0 */
};

struct gpioglobalstatus {
	u32	dummy;
};

struct gpiointerrupt {
	u32	port0;
	u32	port1;
};

struct gpiovolatileinfo {
	u32	dummy;
};

struct hwd_gpio {
	u32	portid;
	void __iomem	*reg_base; /* mapped mem or io address*/

	/* Driver Core Data */

};

static void hwdgpio_initialize0(
	IN struct hwd_gpio *this)
{
	/* int retval; return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	this->reg_base = NULL;
}

static void hwdgpio_setmappedaddress(
	IN struct hwd_gpio *this,
	IN void __iomem *reg_base)
{
	this->reg_base = reg_base;
}

static void hwdgpio_opendevice(
	IN struct hwd_gpio *this,
	IN u32 portid)
{
	this->portid = portid;
}

static void hwdgpio_closedevice(
	IN struct hwd_gpio *this)
{
}

static int hwdgpio_inithw(
	IN struct hwd_gpio *this)
{

	u32 reg_control;
	u8 reg_interruptstatus;
	u8 reg_portdirection;
	u8 reg_portdata;

	reg_control = 0;
	lz_write_reg32(
		this->reg_base, LZ_GPIOREGOFFSET_INTCONTROL0, reg_control);
	lz_write_reg32(
		this->reg_base, LZ_GPIOREGOFFSET_INTCONTROL1, reg_control);

	reg_interruptstatus = 0xFF;
	lz_write_reg8(this->reg_base,
		LZ_GPIOREGOFFSET_INTERRUPTSTATUS0, reg_interruptstatus);
	lz_write_reg8(this->reg_base,
		LZ_GPIOREGOFFSET_INTERRUPTSTATUS1, reg_interruptstatus);

	reg_portdirection = 0;
	lz_write_reg8(this->reg_base,
		LZ_GPIOREGOFFSET_PORTDIRECTION0, reg_portdirection);
	lz_write_reg8(this->reg_base,
		LZ_GPIOREGOFFSET_PORTDIRECTION1, reg_portdirection);

	reg_portdata = 0;
	lz_write_reg8(this->reg_base,
		LZ_GPIOREGOFFSET_PORTDATA0, reg_portdata);
	lz_write_reg8(this->reg_base,
		LZ_GPIOREGOFFSET_PORTDATA1, reg_portdata);
	return 0;
}

static int hwdgpio_isr(
	IN struct hwd_gpio *this,
	OUT u32 *pbinterrupt,
	OUT struct gpiointerrupt *pgpiointerrupt,
	OUT struct gpiovolatileinfo  *pgpiovolatileinfo)
{
	int retval; /* return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */
	u16 int_status;
	u8 reg_interruptstatus0;
	u8 reg_interruptstatus1;
	u8 reg_interruptmask0;
	u8 reg_interruptmask1;

	retval = 0;

	*pbinterrupt = FALSE;
	
#if 1

	reg_interruptmask0 = lz_read_reg8(
		this->reg_base, LZ_GPIOREGOFFSET_INTCONTROL0);
	reg_interruptmask1 = lz_read_reg8(
		this->reg_base, LZ_GPIOREGOFFSET_INTCONTROL1);

	lz_write_reg8(this->reg_base,
		LZ_GPIOREGOFFSET_INTCONTROL0, 0);
	lz_write_reg8(this->reg_base,
		LZ_GPIOREGOFFSET_INTCONTROL1, 0);
#endif
	reg_interruptstatus0 = lz_read_reg8(
		this->reg_base, LZ_GPIOREGOFFSET_INTERRUPTSTATUS0);
	reg_interruptstatus1 = lz_read_reg8(
		this->reg_base, LZ_GPIOREGOFFSET_INTERRUPTSTATUS1);

	/* to quickly handle interrupts. */
	/* reg_interruptstatus & STATUS_INTERRUPT_MASK */
	if ((reg_interruptstatus0 & STATUS_INTERRUPT_MASK) == 0 &&
		(reg_interruptstatus1 & STATUS_INTERRUPT_MASK) == 0) {
#if 1
		lz_write_reg8(this->reg_base,
			LZ_GPIOREGOFFSET_INTCONTROL0, reg_interruptmask0);
		lz_write_reg8(this->reg_base,
			LZ_GPIOREGOFFSET_INTCONTROL1, reg_interruptmask1);
#endif			
		goto funcexit;
	} else {
		*pbinterrupt = TRUE;
	}

	*pbinterrupt = TRUE;
	/* quickly clear interrupt that are handled. */
	lz_write_reg8(this->reg_base,
		LZ_GPIOREGOFFSET_INTERRUPTSTATUS0, reg_interruptstatus0);
	lz_write_reg8(this->reg_base,
		LZ_GPIOREGOFFSET_INTERRUPTSTATUS1, reg_interruptstatus1);
#if 1
	lz_write_reg8(this->reg_base,
		LZ_GPIOREGOFFSET_INTCONTROL0, reg_interruptmask0);
	lz_write_reg8(this->reg_base,
		LZ_GPIOREGOFFSET_INTCONTROL1, reg_interruptmask1);
#endif
		
	pgpiointerrupt->port0 = reg_interruptstatus0;
	pgpiointerrupt->port1 = reg_interruptstatus1;


	goto funcexit;
funcexit:

	return retval;
}

/*static int hwdgpio_dpcforisr(
 *	IN struct hwd_gpio *this,
 *	IN struct gpiointerrupt *pgpiointerrupt,
 *	IN struct gpiovolatileinfo  *pgpiovolatileinfo)
 *{
 *	return 0;
 *}
 *
 *
 *static void hwdgpio_getinterruptstatus(
 *	IN struct hwd_gpio *this,
 *	IN OUT u8 *preg_interruptstatus,
 *	IN OUT struct gpiointerrupt *pgpiointerruptstatus)
 *{
 *}
 *
 *static void hwdgpio_clearinterruptstatus(
 *	IN struct hwd_gpio *this,
 *	IN u8 reg_interruptstatus)
 *{
 *}
 *
 *static void hwdgpio_enableinterrupt(
 *	IN struct hwd_gpio *this,
 *	IN struct gpiointerrupt *pgpiointerrupt)
 *{
 *}
 *
 *static void hwdgpio_disableinterrupt(
 *	IN struct hwd_gpio *this,
 *	IN struct gpiointerrupt *pgpiointerrupt)
 *{
 *}
 */

static void hwdgpio_setglobalconfigure(
	IN struct hwd_gpio *this,
	IN u32 portnumber,
	IN struct gpioglobalconfigure *pglobalconf)
{
	u32 reg_control;
	u8 reg_portdirection;
	u32 regoffset;

	if (portnumber == 0)
		regoffset = LZ_GPIOREGOFFSET_INTCONTROL0;
	else
		regoffset = LZ_GPIOREGOFFSET_INTCONTROL1;
	reg_control = 0;
	_setdatafield(reg_control, INTCONTROL_ENABLECONTROL,
		pglobalconf->enablecontrol);
	_setdatafield(reg_control, INTCONTROL_MASK,
		pglobalconf->mask);
	_setdatafield(reg_control, INTCONTROL_TRIGGERLEVEL,
		pglobalconf->triggerlevel);
	_setdatafield(reg_control, INTCONTROL_KEEPPERIOD,
		pglobalconf->keepperiod);
	_setdatafield(reg_control, INTCONTROL_REPEATTRIGGER,
		pglobalconf->repeattrigger);
	lz_write_reg32(this->reg_base, regoffset, reg_control);

	if (portnumber == 0)
		regoffset = LZ_GPIOREGOFFSET_PORTDIRECTION0;
	else
		regoffset = LZ_GPIOREGOFFSET_PORTDIRECTION1;
	reg_portdirection = pglobalconf->direction;
	lz_write_reg8(this->reg_base, regoffset, reg_portdirection);
}

/*static void hwdgpio_hardreset(
 *	IN struct hwd_gpio *this)
 *{
 *}
 *
.*static void hwdgpio_start(
 *	IN struct hwd_gpio *this)
 *{
 *}
 *
 *static void hwdgpio_stop(
 *	IN struct hwd_gpio *this)
 *{
 *}
 */

static void hwdgpio_getpinvalue(
	IN struct hwd_gpio *this,
	IN u32 portnumber,
	IN u32 pinnumber,
	OUT u32 *pdata1bit)
{
	u8 reg_portdata;
	u32 regoffset;

	if (portnumber == 0)
		regoffset = LZ_GPIOREGOFFSET_PORTDATA0;
	else
		regoffset = LZ_GPIOREGOFFSET_PORTDATA1;
	reg_portdata = lz_read_reg8(
		this->reg_base, regoffset);
	*pdata1bit = (reg_portdata >> pinnumber) & 0x01;
}

static void hwdgpio_setpinvalue(
	IN struct hwd_gpio *this,
	IN u32 portnumber,
	IN u32 pinnumber,
	IN u32 data1bit)
{
	u8 reg_portdata;
	u32 regoffset;

	if (portnumber == 0)
		regoffset = LZ_GPIOREGOFFSET_PORTDATA0;
	else
		regoffset = LZ_GPIOREGOFFSET_PORTDATA1;
	reg_portdata = lz_read_reg8(
		this->reg_base, regoffset);
	if (data1bit == 0)
		reg_portdata &= ~(1 << pinnumber);
	else
		reg_portdata |= (1 << pinnumber);
	lz_write_reg8(this->reg_base, regoffset, reg_portdata);
}

struct spevent {
	struct completion	event;
	spinlock_t		lock;
	u32			trigger;
};

/*
 * ----
 * define this module
 */
struct module;
struct module_driver_data;
struct module_port_device;
/*struct file_operations;*/

static int port_startup(struct module_port_device *this_port_device);
static void port_shutdown(struct module_port_device *this_port_device);
static void port_pm(
	struct module_port_device *this_port_device,
	unsigned int state,
	unsigned int oldstate);

struct module_driver_data {
	struct module	*owner;
	const char	*driver_name;
	const char	*dev_name;
		/* device file name,alloc_chrdev_region() */

	unsigned int	numberdevicefile;
		/* number of device file, chardev.count */

	unsigned int	major; /* MAJOR(chardev.dev); */
	unsigned int	minor_start; /* MINOR(chardev.dev); */
};

#define INTERRUPTQUEUE_MAXCOUNT		10
struct interrupt_extension {
	/*
	 * DpcQueue;
	 * Queue size depend on the frequency of interrupt.
	 * Dont let the queue be full
	 */

	struct gpiointerrupt	losinterrupt[INTERRUPTQUEUE_MAXCOUNT];
	struct gpiovolatileinfo	losvolatileinfo[INTERRUPTQUEUE_MAXCOUNT];
	u32	start;
	u32	end;
	u32	count;
	u32	overrun;
	u32	id; /* sequential id*/

	u32	dummy;
};

#define DEVTYPE_PCIDEV		0
#define DEVTYPE_PLATFORMDEV	1
#define DEVTYPE_PNPDEV		2
struct module_port_device {
	int			bused;
	unsigned int		devtype;
	void			*thisdev;/*alias: this_device*/
	/*struct pci_dev		*pcidev;*/
	/*struct platform_device	*platformdev;*/
	/*struct pnp_dev		*pnpdev;*/

	unsigned int		portnumber;
		/* zero based, mark on multiple port card */

	int			irq;
	unsigned int		iospace;
	unsigned long		iolength;
	unsigned long		baseaddress;
	void __iomem		*reg_base; /* mapped io or mem address */
	/*void __iomem		*reg_base_dma; mapped address DMA*/

	/* linux core device & driver */
	unsigned int		major;
	unsigned int		deviceid;
		/* minor number zero based. tty0 tty1
		 * if device have no minor, let deviceid = moduleportindex.
		 */

	unsigned int		opencount;  /* devcie file open flag */

	/* use chardev if need */
	/* struct cdev chardevicefile;*/

	/* struct device ThisCoreDevice;
	 * module_register_thiscoredev will assign this value.
	 */
	struct gpio_chip	gpiochip;
		/*container_of(adapter, struct module_port_device, sadapter);*/
	char			*coredev_name;
		/* dev_name(ThisCoreDevice.dev) */

	/* device capability */
	unsigned int		numgpio;

	/* monitor device status */

	/* device handles and resources */
	struct hwd_gpio		shwdgpio;
	struct hwd_gpio		*hwdgpio;
	struct gpiointerrupt	sgpiointerrupt;

	spinlock_t			gpiolock;
	struct gpioglobalconfigure	sglobalconf[2];/* two ports*/

	struct list_head	shared_irq_entry;
		/* shared_irq_entry will link all IRQs this module used.*/

	/*struct timer_list	timer_noirq;*/

	struct tasklet_struct	tasklet_isrdpc;

	/* Synchronization */

	/* ISRDPC Queue and Rountine */
	spinlock_t		interruptlock_irq;
		/* for some operation that will generate interrupt */

	spinlock_t		interruptlock_dpcqueue;
	struct interrupt_extension sinterrupt_extension;
		/* to protect DPCQueue and sinterruptExtension. */

	spinlock_t		spinlock_dpc;
		/* using SPSpinLock to protect resources shared with DPC. */

	/*struct spevent		spevent_null;*/
		/* use this to make sure schedule and wait */

	/* interrupt DPC events */
	/*struct spevent		spevent_transmitdone;*/

	/* per-port callback function. */
	int (*startup)(struct module_port_device *this_port_device);
		/* EvtDevicePrepareHardware, EvtDeviceD0Entry,*/
		/* EvtDeviceD0EntryPostInterruptsEnabled */

	void (*shutdown)(struct module_port_device *this_port_device);
		/* EvtDeviceD0ExitPreInterruptsDisabled, EvtDeviceD0Exit,*/
		/* EvtDeviceReleaseHardware */

	void (*pm)(struct module_port_device *this_port_device,
		unsigned int state, unsigned int oldstate);
};

/*
 * ----
 * define sharing interrupt list
 * PS: we may repeatedly register IRQHandler for each port.
 */
struct shared_irq_list {
	spinlock_t		lock;
	unsigned int		referencecount;
	struct list_head	head;
};

static struct shared_irq_list thismodule_shared_irq_lists[NR_IRQS];
	/* the index is irq number(pcidev->irq).
	 * NR_IRQS is the upper bound of irqs in the platform.
	 * so we have NR_IRQS irq lists.
	 */

/*
 * ----
 * define driver data for this module
 */

#define MAX_MODULE_PORTS     4
	/* totoal number of ports for this module.
	 * each device may have more the one port.
	 * ex: four 8-ports pci card.
	 */

/*static struct module_driver_data thismodule_driver_data = {
 *	.owner			= THIS_MODULE,
 *	.driver_name		= DRIVER_NAME,
 *	.dev_name		= "RDCI2C",
 *	.numberdevicefile	= 0,
 *	.minor_start		= 0,
 *
 *};
 */
/*
 * ----
 * define port config
 */

/*
 * ----
 * define device file ops for this module.
 */
/* static struct file_operations thismodule_ops = {...};*/


/*
 * ----
 * define port device for this module
 */

static DEFINE_MUTEX(thismodule_port_devices_mutex);
static struct module_port_device thismodule_port_devices[MAX_MODULE_PORTS];

/*
 * ----
 * define Device & Driver config
 */
static ssize_t drivername_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf);
static DEVICE_ATTR_RO(drivername); /*.show = drivername_show, .store = null*/
	/* dev_attr_drivername */
	/* DEVICE_ATTR_RO DEVICE_ATTR_WO DEVICE_ATTR_RW */
	/* default func name is drivername_show drivername_store */
	/* static DEVICE_ATTR(registers,..); dev_attr_registers */
	/* static DEVICE_ATTR(function,..); dev_attr_function */
	/* static DEVICE_ATTR(queues,..); dev_attr_queues */

static int cb_irq_type(struct irq_data *irqdata, unsigned int type);
static void cb_irq_mask(struct irq_data *irqdata);
static void cb_irq_unmask(struct irq_data *irqdata);
static struct irq_chip this_irq_chip = {
	.name		= "RDCGPIO_irqchip",
	.irq_mask	= cb_irq_mask,
	.irq_unmask	= cb_irq_unmask,
	.irq_set_type	= cb_irq_type,
};

static int cb_gpio_request(struct gpio_chip *gpiochip, unsigned int offset);
static void cb_gpio_free(struct gpio_chip *gpiochip, unsigned int offset);
static int cb_gpio_direction_input(
	struct gpio_chip *gpiochip, unsigned int offset);
static int cb_gpio_direction_output(
	struct gpio_chip *gpiochip, unsigned int offset, int value);
static int cb_gpio_get(struct gpio_chip *gpiochip, unsigned int offset);
static void cb_gpio_set(
	struct gpio_chip *gpiochip, unsigned int offset, int value);

static irqreturn_t IRQHandler(int irq, void *devicedata);
static void routine_isrdpc(unsigned long moduleportindex);

static int moduleport_init(void)
{
	int retval; /* return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	unsigned int i;

	pr_info("%s:\n", __func__);

	/* initialize module level (global) data. */
	retval = 0;
	for (i = 0; i < NR_IRQS; i++) {
		spin_lock_init(&thismodule_shared_irq_lists[i].lock);
		thismodule_shared_irq_lists[i].referencecount = 0;
		INIT_LIST_HEAD(&thismodule_shared_irq_lists[i].head);
	}

	pr_info("    this module supprts %u ports\n", MAX_MODULE_PORTS);
	for (i = 0; i < MAX_MODULE_PORTS; i++)
		thismodule_port_devices[i].bused = FALSE;

	return retval;
}

static void moduleport_exit(void)
{
	pr_info("%s:\n", __func__);
}

static ssize_t drivername_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	return sprintf(buf, "%s\n", DRIVER_NAME);
}

static int module_shared_irq_reference(int irq)
{
	int retval; /* return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	struct shared_irq_list *this_shared_irq_list;

	retval = 0;

	if (!is_real_interrupt(irq)) {
		pr_warn("    No IRQ.  Check hardware Setup!\n");
		goto funcexit;
	}

	this_shared_irq_list = &thismodule_shared_irq_lists[irq];
	spin_lock_irq(&this_shared_irq_list->lock);
	if (this_shared_irq_list->referencecount == 0) {
		this_shared_irq_list->referencecount = 1;
		spin_unlock_irq(&this_shared_irq_list->lock);
		/* PS: request_irq is sleeping function.*/
		if (request_irq(irq, IRQHandler, IRQF_SHARED, DRIVER_NAME,
			this_shared_irq_list) != 0) {
			pr_err("    request interrupt %i failed\n", irq);
			retval = -EFAULT;
			goto funcexit;
		} else {
			pr_info("    request IRQ %i handler success\n", irq);
		}
	} else {
		this_shared_irq_list->referencecount += 1;
		spin_unlock_irq(&this_shared_irq_list->lock);
		pr_info("    this_shared_irq_list->referencecount += 1;\n");
	}

	goto funcexit;
funcexit:

	return retval;
}

static int module_shared_irq_dereference(int irq)
{
	int retval; /* return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	struct shared_irq_list *this_shared_irq_list;

	retval = 0;

	if (!is_real_interrupt(irq))
		goto funcexit;

	this_shared_irq_list = &thismodule_shared_irq_lists[irq];
	spin_lock_irq(&this_shared_irq_list->lock);
	this_shared_irq_list->referencecount -= 1;
	if (this_shared_irq_list->referencecount == 0) {
		/* PS: free_irq is sleeping function.*/
		spin_unlock_irq(&this_shared_irq_list->lock);
		free_irq(irq, this_shared_irq_list);
		pr_info("    free_irq %i\n", irq);
	} else {
		spin_unlock_irq(&this_shared_irq_list->lock);
	}

	pr_info("    this_shared_irq_list->referencecount -= 1;\n");

	goto funcexit;
funcexit:

	return retval;
}

static int module_register_port_device(
	unsigned int portnumber,
	unsigned int *pmoduleportindex)
{
	int retval; /* return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	unsigned int i;
	struct module_port_device *this_port_device;

	pr_info("%s Entry\n", __func__);
	retval = 0;
	*pmoduleportindex = 0;

	mutex_lock(&thismodule_port_devices_mutex);
	for (i = 0; i < MAX_MODULE_PORTS; i++) {
		if (thismodule_port_devices[i].bused == FALSE) {
			/* init port device */
			this_port_device = &thismodule_port_devices[i];
			this_port_device->portnumber = portnumber;
			this_port_device->bused = TRUE;
			*pmoduleportindex = i;
			break;
		}
	}
	mutex_unlock(&thismodule_port_devices_mutex);

	if (i == MAX_MODULE_PORTS) {
		pr_warn("    there is no non-used module port\n");
		retval = -EFAULT;
	}

	goto funcexit;
funcexit:

	pr_info("%s Exit\n", __func__);
	return retval;
}

static int module_initialize_port_device(
	unsigned int moduleportindex,
	unsigned int devtype,
	void *pdev)
{
	int retval; /* return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	struct module_port_device *this_port_device;

	pr_info("%s Entry moduleportindex:%u\n", __func__, moduleportindex);
	retval = 0;
	this_port_device = &thismodule_port_devices[moduleportindex];

	this_port_device->devtype = devtype;
	this_port_device->thisdev = pdev;
	this_port_device->opencount = 0;
	this_port_device->major = 0;
	this_port_device->deviceid = 0;
	/*this_port_device->gpiochip = NULL;*/
	this_port_device->hwdgpio = &this_port_device->shwdgpio;
	hwdgpio_initialize0(this_port_device->hwdgpio);

	spin_lock_init(&this_port_device->gpiolock);
	memset(&this_port_device->sglobalconf[0], 0,
		sizeof(struct gpioglobalconfigure));
	memset(&this_port_device->sglobalconf[1], 0,
		sizeof(struct gpioglobalconfigure));

	tasklet_init(&this_port_device->tasklet_isrdpc, routine_isrdpc,
		(unsigned long)moduleportindex);
	spin_lock_init(&this_port_device->interruptlock_irq);
	spin_lock_init(&this_port_device->interruptlock_dpcqueue);
	memset(&this_port_device->sinterrupt_extension, 0,
		sizeof(struct interrupt_extension));

	spin_lock_init(&this_port_device->spinlock_dpc);

	/*spevent_initialize(&this_port_device->spevent_null);*/

	/* init per-port callback function. */
	this_port_device->startup = port_startup;
	this_port_device->shutdown = port_shutdown;
	this_port_device->pm = port_pm;

	goto funcexit;
funcexit:

	pr_info("%s Exit\n", __func__);
	return retval;
}

static int module_ioattach_port_device(
	unsigned int moduleportindex,
	int irq,
	unsigned int iospace,
	unsigned long iolength,
	unsigned long baseaddress,
	void __iomem *reg_base)
{
	int retval; /* return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	struct module_port_device *this_port_device;

	pr_info("%s Entry moduleportindex:%u\n", __func__, moduleportindex);
	retval = 0;
	this_port_device = &thismodule_port_devices[moduleportindex];

	/* keep io address & irq */
	this_port_device->iospace = iospace;
	this_port_device->iolength = iolength;
	this_port_device->baseaddress = baseaddress;
	this_port_device->reg_base = reg_base;
	this_port_device->irq = irq;

	pr_info("    this_port_device:x%p\n", this_port_device);
	pr_info("    this_port_device->iospace:%u\n",
		this_port_device->iospace);
	pr_info("    this_port_device->baseaddress:x%lx\n",
		this_port_device->baseaddress);
	pr_info("    this_port_device->reg_base:x%p\n",
		this_port_device->reg_base);

	goto funcexit;
funcexit:

	pr_info("%s Exit\n", __func__);
	return retval;
}

static int module_setting_port_device(
	unsigned int moduleportindex,
	unsigned int numgpio)
{
	int retval; /* return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	struct module_port_device *this_port_device;

	pr_info("%s Entry moduleportindex:%u\n", __func__, moduleportindex);
	retval = 0;
	this_port_device = &thismodule_port_devices[moduleportindex];

	this_port_device->numgpio = numgpio;

	goto funcexit;
funcexit:

	pr_info("%s Exit\n", __func__);
	return retval;
}

static int module_setup_port_device(
	unsigned int moduleportindex)
{
	int retval; /* return -ERRNO */
	int reterr; /* check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	struct module_port_device *this_port_device;
	struct shared_irq_list    *this_shared_irq_list;

	pr_info("%s Entry moduleportindex:%u\n", __func__, moduleportindex);
	retval = 0;
	this_port_device = &thismodule_port_devices[moduleportindex];

	/* call startup callback */
	reterr = this_port_device->startup(this_port_device);
	if (reterr < 0) {
		pr_err("    this_port_device->startup failed\n");

		mutex_lock(&thismodule_port_devices_mutex);
		this_port_device->bused = FALSE;
		mutex_unlock(&thismodule_port_devices_mutex);

		retval = -EFAULT;
		goto funcexit;
	}
	/* link shared_irq_entry to shared_irq_list */
	INIT_LIST_HEAD(&this_port_device->shared_irq_entry);
	if (is_real_interrupt(this_port_device->irq)) {
		/* link_irq_chain() */
		this_shared_irq_list =
			&thismodule_shared_irq_lists[this_port_device->irq];

		spin_lock_irq(&this_shared_irq_list->lock);
		list_add_tail(&this_port_device->shared_irq_entry,
			&this_shared_irq_list->head);
		spin_unlock_irq(&this_shared_irq_list->lock);

		pr_info("    link shared_irq_entry to shared_irq_list\n");
	}

	goto funcexit;
funcexit:

	pr_info("%s Exit\n", __func__);
	return retval;
}

static int module_shutdown_port_device(
	unsigned int moduleportindex)
{
	int retval; /* return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	struct module_port_device *this_port_device;
	struct shared_irq_list    *this_shared_irq_list;

	pr_info("%s Entry moduleportindex:%u\n", __func__, moduleportindex);
	retval = 0;
	this_port_device = &thismodule_port_devices[moduleportindex];

	/* unlink shared_irq_entry from shared_irq_list */
	if (is_real_interrupt(this_port_device->irq)) {
		/* unlink_irq_chain() */
		this_shared_irq_list =
			&thismodule_shared_irq_lists[this_port_device->irq];

		spin_lock_irq(&this_shared_irq_list->lock);
		list_del(&this_port_device->shared_irq_entry);
		spin_unlock_irq(&this_shared_irq_list->lock);

		pr_info("    unlink shared_irq_entry from shared_irq_list\n");
	}

	/* shutdown */
	this_port_device->shutdown(this_port_device);

	tasklet_kill(&this_port_device->tasklet_isrdpc);

	goto funcexit;
funcexit:

	pr_info("%s Exit\n", __func__);
	return retval;
}


static int module_unregister_port_device(
	unsigned int moduleportindex)
{
	int retval; /* return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	struct module_port_device *this_port_device;

	pr_info("%s Entry moduleportindex:%u\n", __func__, moduleportindex);
	retval = 0;
	this_port_device = &thismodule_port_devices[moduleportindex];

	/* unregister port device */
	mutex_lock(&thismodule_port_devices_mutex);
	this_port_device->bused = FALSE;
	mutex_unlock(&thismodule_port_devices_mutex);

	goto funcexit;
funcexit:

	pr_info("%s Exit\n", __func__);
	return retval;
}

static int module_register_thiscoredev(
	unsigned int moduleportindex,
	struct device *dev)
{
	int retval; /* return -ERRNO */
	int reterr; /* check if (reterr < 0) */
	unsigned int result; /* check if (result != 0) */

	struct module_port_device	*this_port_device;
	struct gpio_chip		*this_gpio_chip;	
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0))	
	struct gpio_irq_chip *girq;
#endif


	
	pr_info("%s Entry moduleportindex:%u\n", __func__, moduleportindex);
	retval = 0;

	pr_info("    dev_name %s\n", dev_name(dev)); /* ex: 0000:03:06.0 */

	/* pcidev = container_of(dev, struct pci_dev, device); */
	this_port_device = &thismodule_port_devices[moduleportindex];
	/* this_driver_data = dev_get_drvdata(dev); */
	/* this_specific = this_driver_data->this_specific; */

	/* allocate ThisCoreDevice */
	/*this_port_device->gpiochip = &thismodule_gpiochip[moduleportindex];*/

	/* init ThisCoreDevice */
	this_gpio_chip = &this_port_device->gpiochip;
	memset(this_gpio_chip, 0, sizeof(struct gpio_chip));
	pr_info("    thisgpiochip.label %s\n", dev_name(dev)); /*0000:03:06.0*/
	this_gpio_chip->label = dev_name(dev);
		/* gpiod_find need label for lookup table */
	this_gpio_chip->parent = dev;
	this_gpio_chip->base = -1;
	this_gpio_chip->ngpio = this_port_device->numgpio;
	this_gpio_chip->request = cb_gpio_request;
	this_gpio_chip->free = cb_gpio_free;
	this_gpio_chip->direction_input = cb_gpio_direction_input;
	this_gpio_chip->direction_output = cb_gpio_direction_output;
	this_gpio_chip->get = cb_gpio_get;
	this_gpio_chip->set = cb_gpio_set;
	this_gpio_chip->can_sleep = false;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0))
	girq = &this_gpio_chip->irq;
	girq->chip = &this_irq_chip;
	/* This will let us handle the parent IRQ in the driver */
	girq->parent_handler = NULL;
	girq->num_parents = 0;
	girq->parents = NULL;
	girq->default_type = IRQ_TYPE_NONE;
	girq->handler = handle_simple_irq;	
#endif	
	/* register ThisCoreDevice */
	result = gpiochip_add(this_gpio_chip);
	if (result != 0) {
		pr_err("    gpiochip_add failed %i\n", result);
		retval = -EFAULT;
		goto funcexit;
	}	
		
	/* now, register is ok */
	/* dev equal to this_gpio_chip->parent*/
	/* maybe we can use this_gpio_chip->gpiodev->dev */
	this_port_device->coredev_name = (char *)dev_name(dev);
	pr_info("    register ThisCoreDevice ok, dev_name:%s\n",
		this_port_device->coredev_name);
	pr_info("    gpiochip base:%i\n", this_gpio_chip->base);
	if (MAJOR(dev->devt) != 0) {
		this_port_device->major = MAJOR(dev->devt);
		this_port_device->deviceid = MINOR(dev->devt);
		pr_info("    now, this port is registered with major:%u deviceid:%u\n",
			this_port_device->major, this_port_device->deviceid);
	} else {
		/*this_port_device->major = 0;
		 *this_port_device->deviceid = moduleportindex;
		 *pr_info("    this port have no major, so we let deviceid "
		 *"equal to moduleportindex:%u\n", this_port_device->deviceid);
		 */
		this_port_device->major = 0;
		this_port_device->deviceid = moduleportindex;
		pr_info("    this port have no major, so we let deviceid equal to moduleportindex:%u\n",
			this_port_device->deviceid);
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0))
	result = gpiochip_irqchip_add(this_gpio_chip,
		&this_irq_chip, 0, handle_simple_irq, IRQ_TYPE_NONE);
	if (result != 0) {
		pr_info("    gpiochip_irqchip_add failed %i\n", result);
		retval = -EFAULT;
		goto funcexit;
	}	
#endif	

	module_shared_irq_reference(this_port_device->irq);

	/* create device attributes for ThisCoreDevice */
	reterr = device_create_file(
		dev, &dev_attr_drivername);
	if (reterr < 0) {
		pr_err("    device_create_file failed %i\n", reterr);
		retval = -EFAULT;
		goto funcexit;
	}
	/* reterr = device_create_file(dev_attr_others); */

	goto funcexit;
funcexit:

	if (retval != 0) {
		module_shared_irq_dereference(this_port_device->irq);
		if (this_port_device->gpiochip.gpiodev != NULL)
			gpiochip_remove(&this_port_device->gpiochip);
		this_port_device->gpiochip.parent = NULL;
	}

	pr_info("%s Exit\n", __func__);
	return retval;
}

static int module_unregister_thiscoredev(
	unsigned int moduleportindex)
{
	int retval; /* return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	struct module_port_device *this_port_device;

	pr_info("%s Entry moduleportindex:%u\n", __func__, moduleportindex);
	retval = 0;
	this_port_device = &thismodule_port_devices[moduleportindex];

	if (this_port_device->gpiochip.gpiodev != NULL) {
		device_remove_file(this_port_device->gpiochip.parent,
			&dev_attr_drivername);

		/* unregister ThisCoreDevice */
		module_shared_irq_dereference(this_port_device->irq);
		gpiochip_remove(&this_port_device->gpiochip);

		this_port_device->gpiochip.parent = NULL;
	}

	goto funcexit;
funcexit:

	pr_info("%s Exit\n", __func__);
	return retval;
}

static int module_suspend_thiscoredev(
	unsigned int moduleportindex)
{
	int retval; /* return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	struct module_port_device *this_port_device;

	pr_info("%s Entry moduleportindex:%u\n", __func__, moduleportindex);
	retval = 0;
	this_port_device = &thismodule_port_devices[moduleportindex];

	if (this_port_device->opencount == 1)
		/* stop queue */

		/* wait for ISRDPC done ??? */

		/* abort and wait for all transmission */

		/* suspend device */
		;

	goto funcexit;
funcexit:

	pr_info("%s Exit\n", __func__);
	return retval;
}

static int module_resume_thiscoredev(
	unsigned int moduleportindex)
{
	int retval; /* return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	struct module_port_device *this_port_device;

	pr_info("%s Entry moduleportindex:%u\n", __func__, moduleportindex);
	retval = 0;
	this_port_device = &thismodule_port_devices[moduleportindex];

	if (this_port_device->opencount == 1)
		/* restart device */
		;

	goto funcexit;
funcexit:

	pr_info("%s Exit\n", __func__);
	return retval;
}

static int port_startup(
	struct module_port_device *this_port_device)
{
	int retval; /* return -ERRNO */
	int reterr; /* check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	retval = 0;

	/* EvtDevicePrepareHardware */
	hwdgpio_setmappedaddress(this_port_device->hwdgpio,
		this_port_device->reg_base);

	hwdgpio_opendevice(
		this_port_device->hwdgpio, this_port_device->portnumber);

	reterr = hwdgpio_inithw(this_port_device->hwdgpio);
	if (reterr < 0) {
		pr_err("    hwdgpio_inithw error:x%X\n", reterr);
		retval = -EFAULT;
		goto funcexit;
	} else {
		/* EvtDeviceD0Entry */
		/* hwd_start() */

		/* EvtDeviceD0EntryPostInterruptsEnabled */
		this_port_device->sglobalconf[0].direction = 0x00;
		this_port_device->sglobalconf[0].enablecontrol = 0;
		this_port_device->sglobalconf[0].mask = 0x00;
		this_port_device->sglobalconf[0].triggerlevel = 0x00;
		this_port_device->sglobalconf[0].keepperiod =
			INTERRUPTKEEPPERIOD_2MS;
		this_port_device->sglobalconf[0].repeattrigger = 0x00;

		this_port_device->sglobalconf[1].direction = 0x00;
		this_port_device->sglobalconf[1].enablecontrol = 0;
		this_port_device->sglobalconf[1].mask = 0x00;
		this_port_device->sglobalconf[1].triggerlevel = 0x00;
		this_port_device->sglobalconf[1].keepperiod =
			INTERRUPTKEEPPERIOD_2MS;
		this_port_device->sglobalconf[1].repeattrigger = 0x00;
		/* hwdgpio_enableinterrupt() */
	}

	goto funcexit;
funcexit:

	return retval;
}

static void port_shutdown(
	struct module_port_device *this_port_device)
{
	/* int retval; return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	/* EvtDeviceD0ExitPreInterruptsDisabled */
	/* hwdgpio_DisableInterrupt() */

	/* EvtDeviceD0Exit */
	/* hwdgpio_Stop() */
	/* hwdgpio_HardReset() */

	/* EvtDeviceReleaseHardware */
	hwdgpio_closedevice(this_port_device->hwdgpio);

}

static void port_pm(
	struct module_port_device *this_port_device,
	unsigned int state,
	unsigned int oldstate)
{
	/* int retval; return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

}

static irqreturn_t IRQHandler(int irq, void *devicedata)
{
	int retval; /* return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	struct shared_irq_list      *this_shared_irq_list;
	struct module_port_device   *this_port_device;
	struct interrupt_extension  *pinterrupt_ext;

	unsigned int                interruptserviced;

	/*unsigned int                start;*/
	/*unsigned int                end;*/
	/*unsigned int                count;*/

	unsigned int                binterrupt;
	struct gpiointerrupt         sinterrupt = {0};
	struct gpiovolatileinfo      svolatileinfo = {0};

	unsigned long flags;
	unsigned int i;
	unsigned int offset;


	retval = IRQ_NONE;
	this_shared_irq_list = (struct shared_irq_list *)devicedata;

	spin_lock(&this_shared_irq_list->lock);
	if (list_empty(&this_shared_irq_list->head)) {
		/* pr_info("IRQHandler this_shared_irq_list emtpy\n"); */
		spin_unlock(&this_shared_irq_list->lock);
		goto funcexit;
	}
	list_for_each_entry(this_port_device,
		&this_shared_irq_list->head, shared_irq_entry) {
		interruptserviced = FALSE;
		binterrupt = FALSE;
		pinterrupt_ext = &this_port_device->sinterrupt_extension;

		if (this_port_device->opencount == 0)
			continue;/*cb_gpio_request cb_gpio_free*/

		/* now, handle isr*/
		memset(&sinterrupt, 0, sizeof(sinterrupt));
		memset(&svolatileinfo, 0, sizeof(svolatileinfo));
		binterrupt = FALSE;
		
		hwdgpio_isr(this_port_device->hwdgpio,
			&binterrupt, &sinterrupt, &svolatileinfo);
		
		if (binterrupt == FALSE)
			continue;

		/* pr_info("    binterrupt == TRUE\n"); */
		interruptserviced = TRUE;

		//spin_lock_irqsave(&this_port_device->gpiolock, flags);
		offset = 0;
		for (i = 0; i < 8; i++)
			if ((sinterrupt.port0 >> i) & 0x01) {
				pr_info("....generic_handle_irq offset:%u\n",
					offset + i);
				generic_handle_irq(irq_find_mapping(
					this_port_device->gpiochip.irq.domain,
					offset + i));
			}
		offset = 8;
		for (i = 0; i < 8; i++)
			if ((sinterrupt.port1 >> i) & 0x01) {
				pr_info("....generic_handle_irq offset:%u\n",
					offset + i);
				generic_handle_irq(irq_find_mapping(
					this_port_device->gpiochip.irq.domain,
					offset + i));
			}
		//spin_unlock_irqrestore(&this_port_device->gpiolock, flags);

		/* save InterruptVolatileInformation into DpcQueue */

		if (interruptserviced == TRUE) {
			/* schedule EvtInterruptDpc */
			/*tasklet_schedule(&this_port_device->tasklet_isrdpc);*/
			retval = IRQ_HANDLED;
		}
	}
	spin_unlock(&this_shared_irq_list->lock);

	goto funcexit;
funcexit:

	return retval;
}

static void routine_isrdpc(unsigned long moduleportindex)
{
}

static int cb_gpio_request(
	struct gpio_chip *gpiochip,
	unsigned int offset)
{
	struct module_port_device *this_port_device;

	this_port_device =
		container_of(gpiochip, struct module_port_device, gpiochip);
	this_port_device->opencount = 1;

	return 0;
}


static void cb_gpio_free(
	struct gpio_chip *gpiochip,
	unsigned int offset)
{
	struct module_port_device *this_port_device;

	this_port_device =
		container_of(gpiochip, struct module_port_device, gpiochip);
	this_port_device->opencount = 0;
}

static int cb_gpio_direction_input(
	struct gpio_chip *gpiochip,
	unsigned int offset)
{
	struct module_port_device *this_port_device;

	unsigned int portnumber;
	unsigned int pinnumber;
	unsigned char pinbit;
	unsigned long flags;
	struct gpioglobalconfigure *pgpioconf;

	this_port_device =
		container_of(gpiochip, struct module_port_device, gpiochip);

	pr_info("    %s Entry offset:%u portnumber:%u pinnumber:%u\n",
		__func__, offset, offset / 8, offset % 8);
	portnumber = offset / 8;
	pinnumber = offset % 8;
	pinbit = (0x01 << pinnumber);

	spin_lock_irqsave(&this_port_device->gpiolock, flags);
	pgpioconf = &this_port_device->sglobalconf[portnumber];
	pgpioconf->direction &= ~(unsigned char)(pinbit);
	hwdgpio_setglobalconfigure(
		this_port_device->hwdgpio, portnumber, pgpioconf);
	spin_unlock_irqrestore(&this_port_device->gpiolock, flags);

	return 0;
}

static int cb_gpio_direction_output(
	struct gpio_chip *gpiochip,
	unsigned int offset,
	int value)
{
	struct module_port_device *this_port_device;

	unsigned int portnumber;
	unsigned int pinnumber;
	unsigned char pinbit;
	unsigned long flags;
	struct gpioglobalconfigure *pgpioconf;

	this_port_device =
		container_of(gpiochip, struct module_port_device, gpiochip);

	pr_info("    %s Entry offset:%u portnumber:%u pinnumber:%u value:%u\n",
		__func__, offset, offset / 8, offset % 8, value);
	portnumber = offset / 8;
	pinnumber = offset % 8;
	pinbit = (0x01 << pinnumber);

	spin_lock_irqsave(&this_port_device->gpiolock, flags);
	pgpioconf = &this_port_device->sglobalconf[portnumber];
	pgpioconf->direction |= (unsigned char)(pinbit);
	hwdgpio_setglobalconfigure(
		this_port_device->hwdgpio, portnumber, pgpioconf);
	hwdgpio_setpinvalue(
		this_port_device->hwdgpio, portnumber, pinnumber, value);
	spin_unlock_irqrestore(&this_port_device->gpiolock, flags);

	return 0;
}

static int cb_gpio_get(
	struct gpio_chip *gpiochip,
	unsigned int offset)
{
	struct module_port_device *this_port_device;

	unsigned int portnumber;
	unsigned int pinnumber;
	unsigned int data1bit;

	this_port_device =
		container_of(gpiochip, struct module_port_device, gpiochip);
	portnumber = offset / 8;
	pinnumber = offset % 8;
	data1bit = 0;
	hwdgpio_getpinvalue(
		this_port_device->hwdgpio, portnumber, pinnumber, &data1bit);

	goto funcexit;
funcexit:

	return (int)data1bit;
}

static void cb_gpio_set(
	struct gpio_chip *gpiochip,
	unsigned int offset,
	int value)
{
	struct module_port_device *this_port_device;

	unsigned int portnumber;
	unsigned int pinnumber;
	unsigned int data1bit;

	this_port_device =
		container_of(gpiochip, struct module_port_device, gpiochip);
	portnumber = offset / 8;
	pinnumber = offset % 8;
	data1bit = value;
	hwdgpio_setpinvalue(
		this_port_device->hwdgpio, portnumber, pinnumber, data1bit);
}

static int cb_irq_type(
	struct irq_data *irqdata,
	unsigned int type)
{
	struct module_port_device *this_port_device;
	struct gpio_chip *gpiochip = irq_data_get_irq_chip_data(irqdata);
	unsigned int offset = irqd_to_hwirq(irqdata);
	unsigned int portnumber;
	unsigned int pinnumber;
	unsigned char pinbit;
	unsigned long flags;
	struct gpioglobalconfigure *pgpioconf;

	this_port_device =
		container_of(gpiochip, struct module_port_device, gpiochip);

	if (offset >= this_port_device->gpiochip.ngpio)
		return -EINVAL;

	pr_info("    %s Entry offset:%u portnumber:%u pinnumber:%u\n",
		__func__, offset, offset / 8, offset % 8);
	portnumber = offset / 8;
	pinnumber = offset % 8;
	pinbit = (0x01 << pinnumber);

	switch (type & IRQ_TYPE_SENSE_MASK) {
	case IRQ_TYPE_EDGE_RISING:
	case IRQ_TYPE_EDGE_FALLING:
	case IRQ_TYPE_LEVEL_HIGH:
	case IRQ_TYPE_LEVEL_LOW:
		break;
	default:
		return -EINVAL;
	}

	spin_lock_irqsave(&this_port_device->gpiolock, flags);
	/*triggerlevel = 0x00; 1:low 0:high */
	pgpioconf = &this_port_device->sglobalconf[portnumber];
	switch (type & IRQ_TYPE_SENSE_MASK) {
	case IRQ_TYPE_EDGE_RISING:
		pgpioconf->repeattrigger &= ~(unsigned char)pinbit;
		pgpioconf->triggerlevel &= ~(unsigned char)pinbit;
		pr_info("    IRQ_TYPE_EDGE_RISING\n");
		break;

	case IRQ_TYPE_EDGE_FALLING:
		pgpioconf->repeattrigger &= ~(unsigned char)pinbit;
		pgpioconf->triggerlevel |= (unsigned char)pinbit;
		pr_info("    IRQ_TYPE_EDGE_FALLING\n");
		break;

	case IRQ_TYPE_LEVEL_HIGH:
		pgpioconf->repeattrigger |= (unsigned char)pinbit;
		pgpioconf->triggerlevel &= ~(unsigned char)pinbit;
		pr_info("    IRQ_TYPE_LEVEL_HIGH\n");
		break;

	case IRQ_TYPE_LEVEL_LOW:
		pgpioconf->repeattrigger |= (unsigned char)pinbit;
		pgpioconf->triggerlevel |= (unsigned char)pinbit;
		pr_info("    IRQ_TYPE_LEVEL_LOW\n");
		break;
	}
	pgpioconf->enablecontrol = 1;
	hwdgpio_setglobalconfigure(
		this_port_device->hwdgpio, portnumber, pgpioconf);
	spin_unlock_irqrestore(&this_port_device->gpiolock, flags);

	return 0;
}

static void cb_irq_mask(/* disable irq */
	struct irq_data *irqdata)
{
	struct module_port_device *this_port_device;
	struct gpio_chip *gpiochip = irq_data_get_irq_chip_data(irqdata);
	unsigned int offset = irqd_to_hwirq(irqdata);
	unsigned int portnumber;
	unsigned int pinnumber;
	unsigned char pinbit;
	unsigned long flags;
	struct gpioglobalconfigure *pgpioconf;

	this_port_device =
		container_of(gpiochip, struct module_port_device, gpiochip);

	if (offset >= this_port_device->gpiochip.ngpio)
		return;

	pr_info("    %s Entry offset:%u portnumber:%u pinnumber:%u\n",
		__func__, offset, offset / 8, offset % 8);
	portnumber = offset / 8;
	pinnumber = offset % 8;
	pinbit = (0x01 << pinnumber);

	//spin_lock(&this_port_device->gpiolock);
	spin_lock_irqsave(&this_port_device->gpiolock, flags);
	/*mask = 0xFF; 1:enable interrupt 0:disable interrupt */
	pgpioconf = &this_port_device->sglobalconf[portnumber];
	pgpioconf->mask &= ~(unsigned char)pinbit;
	hwdgpio_setglobalconfigure(
		this_port_device->hwdgpio, portnumber, pgpioconf);
	//spin_unlock(&this_port_device->gpiolock);
	spin_unlock_irqrestore(&this_port_device->gpiolock, flags);
}

static void cb_irq_unmask(/* enable irq */
	struct irq_data *irqdata)
{
	struct module_port_device *this_port_device;
	struct gpio_chip *gpiochip = irq_data_get_irq_chip_data(irqdata);
	unsigned int offset = irqd_to_hwirq(irqdata);
	unsigned int portnumber;
	unsigned int pinnumber;
	unsigned char pinbit;
	unsigned long flags;
	struct gpioglobalconfigure *pgpioconf;

	this_port_device =
		container_of(gpiochip, struct module_port_device, gpiochip);

	if (offset >= this_port_device->gpiochip.ngpio)
		return;

	pr_info("    %s Entry offset:%u portnumber:%u pinnumber:%u\n",
		__func__, offset, offset / 8, offset % 8);
	portnumber = offset / 8;
	pinnumber = offset % 8;
	pinbit = (0x01 << pinnumber);

	spin_lock_irqsave(&this_port_device->gpiolock, flags);
	//spin_lock(&this_port_device->gpiolock);
	/*mask = 0xFF; 1:enable interrupt 0:disable interrupt */
	pgpioconf = &this_port_device->sglobalconf[portnumber];
	pgpioconf->mask |= (unsigned char)pinbit;
	hwdgpio_setglobalconfigure(
		this_port_device->hwdgpio, portnumber, pgpioconf);
	//spin_unlock(&this_port_device->gpiolock);
	spin_unlock_irqrestore(&this_port_device->gpiolock, flags);
}

/*
 * ----
 * platform device data
 */
struct platform_device;
struct platform_specific;
struct platform_driver_data;
struct platform_specific {
	/* we use this structure to keep information of the supported devices
	 * and do specific process for the supported devices.
	 */
	unsigned int	vendor;
	unsigned int	device;
	unsigned int	subvendor;
	unsigned int	subdevice;
	unsigned int	revision;
	unsigned int	pid;
		/* Project ID, if need */

	/* use the following callbacks to init/setup/exit for a device. */
	int (*init)(struct platform_driver_data *this_driver_data);
		/* init pci_dev, init pci config*/

	int (*setup)(struct platform_driver_data *this_driver_data);
		/* allocate resoures - io, mem, irq*/

	void (*exit)(struct platform_driver_data *this_driver_data);
		/* free resoures - io, mem, irq */

	/* IO information for this device.*/
	/* we assume one IO BaseAddress can support all ports.*/
	unsigned int	basebar_forports;
		/* in pnp always 0*/
		/* in pci, BaseBar is 0 - 4. none if 255*/
	unsigned int	num_ports;
	unsigned int	first_offset;
	unsigned int	reg_offset;
	unsigned int	reg_shift;
	unsigned int	reg_length;
		/* per port =  start offset + port index * reg_length.*/

	/*unsigned int	BaseBarForDMA; none if 255*/
	/*unsigned int	BaseBarForParport_io;*/
	/*unsigned int	BaseBarForParport_iohi;*/
	unsigned int	numgpio;
};


#define PCI_NUM_BAR_RESOURCES	5
#define MAX_PORT_INDEX		16

struct platform_driver_data {
	/* we assign this struct's pointer to platform_device.drvdata.*/

	/* device linking list*/
	unsigned int	devtype;
	void		*thisdev;/*alias: this_device*/
	/*struct pci_dev			*pcidev;*/
	/*struct platform_device		*platformdev;*/
	/*struct pnp_dev			*pnpdev;*/
	struct platform_specific	*this_specific;

	/* resources driver allocated*/
	struct resource		*hresource[PCI_NUM_BAR_RESOURCES];
	unsigned int		addressspace[PCI_NUM_BAR_RESOURCES];
		/* 0: memory space, 1: io space*/
	unsigned long		baseaddress[PCI_NUM_BAR_RESOURCES];
	unsigned long		baseaddress_length[PCI_NUM_BAR_RESOURCES];
	void __iomem		*mappedaddress[PCI_NUM_BAR_RESOURCES];
	int			irq;

	/* this module will assign each port device a port index (resource) */
	int			module_port_index[MAX_PORT_INDEX];
		/* the value -1 indicate that the port dont assign a device */
};

/*
 * ----
 * Driver Infomation
 *
 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("kevin.haung@rdc.com.tw");
MODULE_DESCRIPTION("RDC Platform GPIO Controller Driver");
MODULE_VERSION(DRIVER_VERSION);

/*
 * ----
 * define module parameters: mp_
 * if perm is 0, no sysfs entry.
 * if perm is S_IRUGO 0444, there is a sysfs entry, but cant altered.
 * if perm is S_IRUGO | S_IWUSR, there is a sysfs entry and can altered by root.
 */

/*
 * ----
 * define multiple pnp devices for this module supports
 */
#define MAX_SPECIFIC	1 /* this module support */
enum platform_specific_num_t {
	platform_17f3_1901 = 0, /* fixed on board */
};

static int platform_default_init(
	struct platform_driver_data *this_driver_data);
static int platform_default_setup(
	struct platform_driver_data *this_driver_data);
static void platform_default_exit(
	struct platform_driver_data *this_driver_data);

static struct platform_specific thismodule_platform_specifics[] = {
	[platform_17f3_1901] = {
		.vendor		= 0x17f3,
		.device		= 0x1901,
		.subvendor	= PCI_ANY_ID,
		.subdevice	= PCI_ANY_ID,

		.init		= platform_default_init,
		.setup		= platform_default_setup,
		.exit		= platform_default_exit,

		.pid		= 0xA9610,
		.basebar_forports = 0,
		.num_ports	= 1,
		.first_offset	= 0,
		.reg_offset	= 0,
		.reg_shift	= 0,
		.reg_length	= 0x20,
		.numgpio	= 16,
	},
};

/*
 * ----
 * define pnp device table for this module supports
 * PNP_ID_LEN = 8
 */
static const struct pnp_device_id pnp_device_id_table[] = {
	{ .id = "PNP1901", .driver_data = platform_17f3_1901, },
	{ .id = "AHC0B10", .driver_data = platform_17f3_1901, },
	{ } /* terminate list */
};
MODULE_DEVICE_TABLE(pnp, pnp_device_id_table);

static const struct platform_device_id platform_device_id_table[] = {
	/* we take driver_data as a specific number(ID). */
	{ .name = "PNP1901", .driver_data = platform_17f3_1901, },
	{ .name = "AHC0B10", .driver_data = platform_17f3_1901, },
	{ } /* terminate list */
};
MODULE_DEVICE_TABLE(platform, platform_device_id_table);

static const struct of_device_id of_device_id_table[] = {
	{ .compatible = "rdc,ahc-gpio", .data = platform_17f3_1901, },
	{},
};
MODULE_DEVICE_TABLE(of, of_device_id_table);

#ifdef CONFIG_ACPI
static const struct acpi_device_id acpi_device_id_table[] = {
	{ "PNP1901", platform_17f3_1901, },
	{ "AHC0B10", platform_17f3_1901, },
	{ },
};
MODULE_DEVICE_TABLE(acpi, acpi_device_id_table);
#endif

/*
 * ----
 * define this pnp driver
 */

static int pnpdevice_probe(
	struct pnp_dev *pnpdev/*this_dev*/,
	const struct pnp_device_id *this_device_id);
static void pnpdevice_remove(
	struct pnp_dev *pnpdev/*this_dev*/);
static int pnpdevice_suspend(
	struct pnp_dev *pnpdev/*this_dev*/,
	pm_message_t state);
static int pnpdevice_resume(
	struct pnp_dev *pnpdev/*this_dev*/);

static struct pnp_driver thismodule_pnp_driver = {
	.name = DRIVER_NAME,
	.id_table = pnp_device_id_table,
	.probe = pnpdevice_probe,
	.remove = pnpdevice_remove,
#ifdef CONFIG_PM
	.suspend = pnpdevice_suspend,
	.resume = pnpdevice_resume,
#endif
};

static int platformdevice_probe(
	struct platform_device *platformdev/*this_dev*/);
static int platformdevice_remove(
	struct platform_device *platformdev/*this_dev*/);
static int platformdevice_suspend(
	struct platform_device *platformdev/*this_dev*/,
	pm_message_t state);
static int platformdevice_resume(
	struct platform_device *platformdev/*this_dev*/);

static struct platform_driver thismodule_platform_driver = {
	/*.owner = THIS_MODULE,*/
	.driver = {
		.name = DRIVER_NAME,
		.of_match_table = of_match_ptr(of_device_id_table),
		.acpi_match_table = ACPI_PTR(acpi_device_id_table),
		/*.pm = &xgene_gpio_pm,*/
	},
	.id_table = platform_device_id_table,
	.probe = platformdevice_probe,
	.remove = platformdevice_remove,
#ifdef CONFIG_PM
	.suspend = platformdevice_suspend,
	.resume = platformdevice_resume,
#endif
};

/*
 * ----
 * Module Block
 */
static int __init
Module_init(void)
{
	int retval; /* return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	pr_info("%s: platform driver version %s\n", __func__, DRIVER_VERSION);
	pr_info("    sizeof(char):%lu sizeof(int):%lu sizeof(long):%lu\n",
		sizeof(char), sizeof(int), sizeof(long));
	pr_info("    sizeof(void *):%lu sizeof(unsigned long):%lu\n",
		sizeof(void *), sizeof(unsigned long));

	/* check module parameters */

	/* override device_specific */

	/* initialize module level (global) data. */

	retval = moduleport_init();
	if (retval < 0) {
		pr_info("    moduleport_init failed\n");
		goto funcexit;
	}

	retval = pnp_register_driver(&thismodule_pnp_driver);
	if (retval < 0) {
		pr_info("    pnp_register_driver failed\n");
		pr_info("    %s failed\n", __func__);
		goto funcexit;
	}
	retval = platform_driver_register(&thismodule_platform_driver);
	if (retval < 0) {
		pr_info("    platform_register_driver failed\n");
		pr_info("    %s failed\n", __func__);
		goto funcexit;
	}
	retval = 0;

	goto funcexit;
funcexit:
	return retval;
}
module_init(Module_init);

static void __exit
Module_exit(void)
{
	pr_info("%s: %s\n", __func__, DRIVER_NAME);
	pnp_unregister_driver(&thismodule_pnp_driver);
	platform_driver_unregister(&thismodule_platform_driver);
	moduleport_exit();
}
module_exit(Module_exit);

/*
 * ----
 * callback functions for platform_driver
 */
static int _basedevice_probe(
	unsigned int devtype,
	void *thisdev/*this_dev*/,
	struct platform_specific *this_specific)
{
	int retval; /* return -ERRNO */
	int reterr; /* check if (reterr < 0) */
	/*unsigned int result; check if (result != 0) */

	unsigned int i;

	struct device *_dev;/*base class*/
	/*struct pci_dev	*pcidev;*/
	struct platform_device	*platformdev;
	struct pnp_dev		*pnpdev;
	struct platform_driver_data *this_driver_data;

	unsigned int basebar;
	unsigned int offset;
	unsigned int iospace;
	unsigned long iolength;
	unsigned long baseaddress;
	void __iomem *mappedaddress;/*mapped io or mem*/
	int moduleportindex;

	retval = 0;
	this_driver_data = NULL;

	/* if this device is supported by this module,
	 * we create releated driver data for it
	 * and create(or assign) a port device
	 */

	/* create driver data */
	this_driver_data = kzalloc(
		sizeof(struct platform_driver_data), GFP_KERNEL);
	if (this_driver_data == NULL) {
		retval = -ENOMEM;
		goto funcexit;
	}

	/* init driver data */
	this_driver_data->devtype = devtype;
	this_driver_data->thisdev = thisdev;
	_dev = NULL;
	switch (devtype) {
	case DEVTYPE_PCIDEV:
		break;
	case DEVTYPE_PLATFORMDEV:
		platformdev = (struct platform_device *)thisdev;
		_dev = &platformdev->dev;
		break;
	case DEVTYPE_PNPDEV:
		pnpdev = (struct pnp_dev *)thisdev;
		_dev = &pnpdev->dev;
		break;
	}
	this_driver_data->this_specific = this_specific;
	for (i = 0; i < PCI_NUM_BAR_RESOURCES; i++) {
		this_driver_data->hresource[i] = NULL;
		this_driver_data->addressspace[i] = 0;
		this_driver_data->baseaddress[i] = 0;
		this_driver_data->baseaddress_length[i] = 0;
		this_driver_data->mappedaddress[i] = NULL;
	}
	for (i = 0; i < MAX_PORT_INDEX; i++)
		this_driver_data->module_port_index[i] = -1;

	/* device init() & setup() */
	pr_info("    platform device init()\n");
	if (this_specific->init)
		this_specific->init(this_driver_data);
	pr_info("    platform device setup()\n");
	if (this_specific->setup) {
		reterr = this_specific->setup(this_driver_data);
		if (reterr < 0) {
			retval = -EINVAL;
			goto funcexit;
		}
	}

	/* create(or assign) a port device for each port on this pci device. */
	pr_info("    create(or assign) a port device for each port\n");
	for (i = 0; i < this_specific->num_ports; i++) {
		/* here, i is port id of ports of this device. */
		pr_info("    port id: %u\n", i);
		offset = this_specific->first_offset +
			i*this_specific->reg_length;
			/* start offset + port index * uart io length. */
		iolength = this_specific->reg_length;
		basebar = this_specific->basebar_forports;
		iospace = this_driver_data->addressspace[basebar];
		baseaddress = this_driver_data->baseaddress[basebar] + offset;
		mappedaddress = this_driver_data->mappedaddress[basebar] +
			offset;

		/* query a module port index for this port.*/
		/* register a module port device for this port.*/
		moduleportindex = 0;
		reterr = module_register_port_device(i, &moduleportindex);
		if (reterr < 0) {
			pr_info("    module_register_port_device failed\n");
			this_driver_data->module_port_index[i] = -1;
			retval = -ENOMEM;
			goto funcexit;
		}
		/* now, register is ok */
		/* assign module port index to this port. */
		this_driver_data->module_port_index[i] = moduleportindex;

		/* config & init port device */
		module_initialize_port_device(
			moduleportindex, devtype, thisdev);

		module_ioattach_port_device(
			moduleportindex, this_driver_data->irq,
			iospace, iolength, baseaddress, mappedaddress);

		module_setting_port_device(moduleportindex,
			this_specific->numgpio);

		/* setup port device */
		reterr = module_setup_port_device(moduleportindex);
		if (reterr < 0) {
			pr_info("    module_setup_port_device failed\n");
			module_unregister_port_device(moduleportindex);
			retval = -EFAULT;
			goto funcexit;
		}
		/*now, setup port is ok */
		reterr = module_register_thiscoredev(
			moduleportindex, _dev);
		if (reterr < 0) {
			pr_info("    module_register_thiscoredev failed\n");
			module_shutdown_port_device(moduleportindex);
			module_unregister_port_device(moduleportindex);
			retval = -EFAULT;
			goto funcexit;
		} else {
		}
	}

	/* create device attributes file for _dev*/
	/*reterr = device_create_file(_dev, &dev_attr_others);*/

	pr_info("    %s Ok\n", __func__);
	goto funcexit;
funcexit:
	if (retval != 0) {
		if (this_driver_data != NULL) {
			kfree(this_driver_data);
			this_driver_data = NULL;
		}
	}
	return retval;
}

static int _basedevice_remove(
	struct platform_driver_data *this_driver_data)
{
	unsigned int i;

	struct platform_specific *this_specific;
	int moduleportindex;

	this_specific = this_driver_data->this_specific;

	for (i = 0; i < this_specific->num_ports; i++) {
		pr_info("    port number: %u\n", i);
		moduleportindex = this_driver_data->module_port_index[i];
		if (moduleportindex == -1) {
			pr_info("    this port have not been assigned to a module port\n");
		} else {
			module_unregister_thiscoredev(moduleportindex);
			/* shutdown port device */
			module_shutdown_port_device(moduleportindex);
			/* unregister port device */
			module_unregister_port_device(moduleportindex);
		}
		this_driver_data->module_port_index[i] = -1;
	}

	/* device exit() */
	pr_info("    platform device exit()\n");
	if (this_specific->exit)
		this_specific->exit(this_driver_data);

	/* free driver data*/
	kfree(this_driver_data);

	return 0;
}

static int _basedevice_suspend(
	struct platform_driver_data *this_driver_data,
	pm_message_t state)
{
	int retval; /* return -ERRNO */
	/*int reterr; check if (reterr < 0) */
	/*unsigned int result; check if (result != 0) */

	unsigned int i;

	struct platform_specific *this_specific;
	int moduleportindex;

	retval = 0;
	this_specific = this_driver_data->this_specific;

	/* suspend each port device */
	for (i = 0; i < this_specific->num_ports; i++) {
		/*pr_info("port id: %u\n", i);*/
		moduleportindex = this_driver_data->module_port_index[i];
		if (moduleportindex >= 0)
			module_suspend_thiscoredev(moduleportindex);
	}

	/* suspend (adapter)device
	 * if this device can wakeup system,
	 *setting wakeup signal from power state.
	 */

	return retval;
}

static int _basedevice_resume(
	struct platform_driver_data *this_driver_data)
{
	int retval; /* return -ERRNO */
	/*int reterr; check if (reterr < 0) */
	/*unsigned int result; check if (result != 0) */

	unsigned int i;

	struct platform_specific *this_specific;
	int moduleportindex;

	retval = 0;
	this_specific = this_driver_data->this_specific;

	/* resume (adapter)device */

	/* resume each port device */
	for (i = 0; i < this_specific->num_ports; i++) {
		/*pr_info("port id: %u\n", i);*/
		moduleportindex = this_driver_data->module_port_index[i];
		if (moduleportindex >= 0)
			module_resume_thiscoredev(moduleportindex);
	}

	return retval;
}

static int platformdevice_probe(
	struct platform_device *platformdev/*this_dev*/)
{
	int retval; /* return -ERRNO */
	/*int reterr; check if (reterr < 0) */
	/*unsigned int result; check if (result != 0) */

	unsigned int specific_id;
	struct platform_specific         *this_specific;
	const struct platform_device_id  *this_device_id;
	const struct acpi_device_id      *this_device_acpi_id;
	/*const struct of_device_id        *this_device_of_id;*/

	pr_info("%s Entry_Debug\n", __func__);
	retval = 0;
	this_specific = NULL;
	/*dev = &platformdev->dev;*/
	this_device_id = platform_get_device_id(platformdev);
	pr_info("    platformdevice->name:%s\n", platformdev->name);

	/* check if we support this device, then get its specific information.*/
	/* we take driver_data as specific number(index).*/
	if (this_device_id != NULL) {
		pr_info("    platform match device\n");
		pr_info("    find and check this platform specific, name:%s ID:%lu\n",
			this_device_id->name, this_device_id->driver_data);
		specific_id = this_device_id->driver_data;
		if (specific_id >= MAX_SPECIFIC) {
			pr_info("    invalid device\n");
			retval = -EINVAL;
			goto funcexit;
		}
		this_specific = &thismodule_platform_specifics[specific_id];
	} else if (platformdev->dev.driver->acpi_match_table != NULL) {
		pr_info("    acpi match device\n");
		this_device_acpi_id = acpi_match_device(
			platformdev->dev.driver->acpi_match_table,
			&platformdev->dev);
		if  (this_device_acpi_id == NULL) {
			pr_info("    no device\n");
			return -ENODEV;
			goto funcexit;
		}
		pr_info("    find and check this platform specific, name:%s ID:%lu\n",
			this_device_acpi_id->id,
			this_device_acpi_id->driver_data);
		specific_id = this_device_acpi_id->driver_data;
		if (specific_id >= MAX_SPECIFIC) {
			pr_info("    invalid device\n");
			retval = -EINVAL;
			goto funcexit;
		}
		this_specific = &thismodule_platform_specifics[specific_id];
	} else if (platformdev->dev.driver->of_match_table != NULL) {
		pr_info("    of match device, not support\n");
		return -ENODEV;
		goto funcexit;
	} else {
		pr_info("    no device\n");
		return -ENODEV;
		goto funcexit;
	}

	retval = _basedevice_probe(DEVTYPE_PLATFORMDEV, platformdev,
		this_specific);
	goto funcexit;
funcexit:
	return retval;
}

static int platformdevice_remove(
	struct platform_device *platformdev/*this_dev*/)
{
	struct platform_driver_data  *this_driver_data;

	pr_info("%s Entry\n", __func__);
	this_driver_data = platform_get_drvdata(platformdev);
	_basedevice_remove(this_driver_data);

	return 0;
}

static int platformdevice_suspend(
	struct platform_device *platformdev/*this_dev*/,
	pm_message_t state)
{
	int retval; /* return -ERRNO */

	struct platform_driver_data *this_driver_data;

	pr_info("%s Entry\n", __func__);
	this_driver_data = platform_get_drvdata(platformdev);
	retval = _basedevice_suspend(this_driver_data, state);

	return retval;
}

static int platformdevice_resume(
	struct platform_device *platformdev/*this_dev*/)
{
	int retval; /* return -ERRNO */

	struct platform_driver_data *this_driver_data;

	pr_info("%s Entry\n", __func__);
	this_driver_data = platform_get_drvdata(platformdev);
	retval = _basedevice_resume(this_driver_data);

	return retval;
}

static int pnpdevice_probe(
	struct pnp_dev *pnpdev/*this_dev*/,
	const struct pnp_device_id *this_device_id)
{
	int retval; /* return -ERRNO */
	/*int reterr; check if (reterr < 0) */
	/*unsigned int result; check if (result != 0) */

	unsigned int specific_id;
	struct platform_specific *this_specific;

	pr_info("%s Entry\n", __func__);
	retval = 0;
	this_specific = NULL;
	pr_info("    pnpdevice->name:%s\n", pnpdev->name);

	/* check if we support this device, then get its specific information.*/
	/* we take driver_data as specific number(index).*/
	if (this_device_id != NULL) {
		pr_info("    pnp match device\n");
		pr_info("    find and check this pnp specific, name:%s ID:%lu\n",
			this_device_id->id, this_device_id->driver_data);
		specific_id = this_device_id->driver_data;
		if (specific_id >= MAX_SPECIFIC) {
			pr_info("    invalid device\n");
			retval = -EINVAL;
			goto funcexit;
		}
		this_specific = &thismodule_platform_specifics[specific_id];
	} else {
		pr_info("    no device\n");
		return -ENODEV;
		goto funcexit;
	}

	retval = _basedevice_probe(DEVTYPE_PNPDEV, pnpdev, this_specific);
	goto funcexit;
funcexit:
	return retval;
}

static void pnpdevice_remove(
	struct pnp_dev *pnpdev/*this_dev*/)
{
	struct platform_driver_data  *this_driver_data;

	pr_info("%s Entry\n", __func__);
	this_driver_data = pnp_get_drvdata(pnpdev);
	_basedevice_remove(this_driver_data);
}

static int pnpdevice_suspend(
	struct pnp_dev *pnpdev/*this_dev*/,
	pm_message_t state)
{
	int retval; /* return -ERRNO */

	struct platform_driver_data *this_driver_data;

	pr_info("%s Entry\n", __func__);
	this_driver_data = pnp_get_drvdata(pnpdev);
	retval = _basedevice_suspend(this_driver_data, state);

	return retval;
}

static int pnpdevice_resume(
	struct pnp_dev *pnpdev/*this_dev*/)
{
	int retval; /* return -ERRNO */

	struct platform_driver_data *this_driver_data;

	pr_info("%s Entry\n", __func__);
	this_driver_data = pnp_get_drvdata(pnpdev);
	retval = _basedevice_resume(this_driver_data);

	return retval;
}

static int platform_default_init(
	struct platform_driver_data *this_driver_data)
{
	int retval; /* return -ERRNO */
	/*int reterr; check if (reterr < 0) */
	/*unsigned int result; check if (result != 0) */

	retval = 0;

	/* assign Driver Data to device */
	switch (this_driver_data->devtype) {
	case DEVTYPE_PCIDEV:
		break;
	case DEVTYPE_PLATFORMDEV:
		platform_set_drvdata(this_driver_data->thisdev,
			this_driver_data);
		break;
	case DEVTYPE_PNPDEV:
		pnp_set_drvdata(this_driver_data->thisdev, this_driver_data);
		break;
	}

	/* initialize this device through pnp or pci configure. */

	return retval;
}

static void _platform_free_io_resoureces(
	struct platform_driver_data *this_driver_data)
{
	unsigned int i;

	struct platform_specific *this_specific;

	unsigned int iospace;
	unsigned int baseaddr;
	unsigned int length;
	void __iomem *mappedaddr;

	this_specific = this_driver_data->this_specific;

	/* free io resoureces */
	for (i = 0; i < PCI_NUM_BAR_RESOURCES; i++) {
		if (i == this_specific->basebar_forports &&
			this_driver_data->hresource[i] != NULL) {
			iospace = this_driver_data->addressspace[i];
			baseaddr = this_driver_data->baseaddress[i];
			length = this_driver_data->baseaddress_length[i];
			mappedaddr = this_driver_data->mappedaddress[i];
			if (iospace == 0) {
				if (mappedaddr != NULL)
					iounmap(mappedaddr);
				release_mem_region(baseaddr, length);
			} else {
				release_region(baseaddr, length);
			}
			this_driver_data->hresource[i] = NULL;
		}
	}

}

static int platform_default_setup(
	struct platform_driver_data *this_driver_data)
{
	int retval; /* return -ERRNO */
	/*int reterr; check if (reterr < 0) */
	/*unsigned int result; check if (result != 0) */

	unsigned int i;

	struct platform_specific *this_specific;
	void *thisdev;

	unsigned int     iospace;
	resource_size_t  baseaddr;
	unsigned long    length;
	void             *mappedaddr;

	struct resource  *pirqresource;
	struct resource  *pioresource;
	struct resource  *presource;

	retval = 0;

	this_specific = this_driver_data->this_specific;
	thisdev = this_driver_data->thisdev;

	/* allocate io resoures */
	pr_info("    allocating io resources\n");
	for (i = 0; i < PCI_NUM_BAR_RESOURCES; i++) {
		pioresource = NULL;
		switch (this_driver_data->devtype) {
		case DEVTYPE_PCIDEV:
			break;
		case DEVTYPE_PLATFORMDEV:
			pioresource = platform_get_resource(thisdev,
				IORESOURCE_IO, i);
			break;
		case DEVTYPE_PNPDEV:
			pioresource = pnp_get_resource(thisdev,
				IORESOURCE_IO, i);
			break;
		}
		if (pioresource != NULL) {
			baseaddr = pioresource->start;
			iospace = 1;
		} else {
			switch (this_driver_data->devtype) {
			case DEVTYPE_PCIDEV:
				break;
			case DEVTYPE_PLATFORMDEV:
				pioresource = platform_get_resource(thisdev,
					IORESOURCE_MEM, i);
				break;
			case DEVTYPE_PNPDEV:
				pioresource = pnp_get_resource(thisdev,
					IORESOURCE_MEM, i);
				break;
			}
			if (pioresource != NULL) {
				baseaddr = pioresource->start;
				iospace = 0;
			} else {
				break;
			}
		}

		if (pioresource->start == 0 && pioresource->end == 0)
			length = 0;
		else
			length = resource_size(pioresource);

		if (i == this_specific->basebar_forports && length) {
			if (iospace == 0) {
				/* memory space */
				/* struct resource *request_mem_region(
				 *	unsigned long, unsigned long, char *);
				 */
				presource = request_mem_region(
					baseaddr, length, NULL);
				if (presource == NULL) {
					pr_info("    request_mem_region presource == NULL\n");
					retval = -EFAULT;
				}

				/* void *ioremap(*/
				/*	unsigned long, unsigned long); */
				mappedaddr = ioremap(
					baseaddr, length);
				if (mappedaddr == NULL) {
					pr_info("    ioremap nocache mappedaddr == NULL\n");
					retval = -EFAULT;
				}
			} else {
				/* io space */
				/* struct resource *request_region(
				 *	unsigned long, unsigned long, char *);
				 */
				presource = request_region(
					baseaddr, length, NULL);
				if (presource == NULL) {
					pr_info("    request_region presource == NULL\n");
					retval = -EFAULT;
				}

				mappedaddr = (void *)(unsigned long)baseaddr;
				/*pr_info("io_lx(%lx)",
				 *	(unsigned long)mappedaddr);
				 *pr_info("io_p(%p)", mappedaddr);
				 */
			}

			this_driver_data->hresource[i] = presource;
			this_driver_data->addressspace[i] = iospace;
			this_driver_data->baseaddress[i] = baseaddr;
			this_driver_data->baseaddress_length[i] = length;
			this_driver_data->mappedaddress[i] = mappedaddr;

			if (retval != 0)
				goto funcexit;

			pr_info("    baseaddress[%u]\n", i);
			pr_info("    addressspace:%u\n", iospace);
			pr_info("    baseaddress:x%llx\n", baseaddr);
			pr_info("    length:%lu\n", length);
			pr_info("    mappedaddr:x%p\n", mappedaddr);

		}
	}

	/* request irq & install IRQ handle */
	this_driver_data->irq = 0;
	pirqresource = NULL;
	switch (this_driver_data->devtype) {
	case DEVTYPE_PCIDEV:
		break;
	case DEVTYPE_PLATFORMDEV:
		pirqresource = platform_get_resource(thisdev,
			IORESOURCE_IRQ, 0);
		break;
	case DEVTYPE_PNPDEV:
		pirqresource = pnp_get_resource(thisdev,
			IORESOURCE_IRQ, 0);
		break;
	}
	if (pirqresource != NULL) {
		this_driver_data->irq = pirqresource->start;
		/*pr_info("    irq flags = x%X\n",*/
		/*	pnp_irq_flags(this_dev, 0));*/
	}
	/*retval = module_shared_irq_reference(this_driver_data->irq);*/

	goto funcexit;
funcexit:

	if (retval != 0)
		_platform_free_io_resoureces(this_driver_data);

	return retval;
}

static void platform_default_exit(
	struct platform_driver_data *this_driver_data)
{
	/* release irq */
	/*module_shared_irq_dereference(this_driver_data->irq);*/

	/* free io resoureces */
	_platform_free_io_resoureces(this_driver_data);

	switch (this_driver_data->devtype) {
	case DEVTYPE_PCIDEV:
		break;
	case DEVTYPE_PLATFORMDEV:
		platform_set_drvdata(this_driver_data->thisdev, NULL);
		break;
	case DEVTYPE_PNPDEV:
		pnp_set_drvdata(this_driver_data->thisdev, NULL);
		break;
	}
}

