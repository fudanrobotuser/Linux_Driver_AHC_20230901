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
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_reg.h>
#include <linux/serial.h>
#include <linux/serial_core.h>

/* our device header */
/*
 * return -ERRNO -EFAULT -ENOMEM -EINVAL
 * device -EBUSY, -ENODEV, -ENXIO
 * parameter -EOPNOTSUPP
 * pr_info pr_warn pr_err
 */
#define DRIVER_NAME		"RDC COM"
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

/*LZ_XXX_REGOFFSET_*/
#define LZ_UART_RO_DIVHLOW		0x00
#define LZ_UART_RO_DIVHIGH		0x01

#define LZ_UART_RO_RECEIVERBUFFER	0x00
#define LZ_UART_RO_TRANSMITTER		0x00
#define LZ_UART_RO_INTENABLE		0x01
#define LZ_UART_RO_INTID		0x02
#define LZ_UART_RO_FIFOCONTROL		0x02
#define LZ_UART_RO_LINECONTROL		0x03
#define LZ_UART_RO_MODEMCONTROL		0x04
#define LZ_UART_RO_LINESTATUS		0x05
#define LZ_UART_RO_MODEMSTATUS		0x06
#define LZ_UART_RO_SCRATCHPAD		0x07

#define INTENABLE_RECEIVEDDATAAVAILABLE		0x00000001
#define INTENABLE_TRANSMITTERHOLDINGEMPTY	0x00000002
#define INTENABLE_LINESTATUS			0x00000004
#define INTENABLE_MODEMSTATUS			0x00000008

#define INTENABLE_RECEIVEDDATAAVAILABLE_SHIFT	(0)
#define INTENABLE_TRANSMITTERHOLDINGEMPTY_SHIFT	(1)
#define INTENABLE_LINESTATUS_SHIFT		(2)
#define INTENABLE_MODEMSTATUS_SHIFT		(3)

#define INTENABLE_RECEIVEDDATAAVAILABLE_RANGE	0x00000001
#define INTENABLE_TRANSMITTERHOLDINGEMPTY_RANGE	0x00000001
#define INTENABLE_LINESTATUS_RANGE		0x00000001
#define INTENABLE_MODEMSTATUS_RANGE		0x00000001

#define INTID_NOPENDING		0x00000001
#define INTID_ID		0x0000000E

#define INTID_NOPENDING_SHIFT	(0)
#define INTID_ID_SHIFT		(1)

#define INTID_NOPENDING_RANGE	0x00000001
#define INTID_ID_RANGE		0x00000007

#define FIFOCONTROL_FIFOENABLE			0x00000001
#define FIFOCONTROL_RXFIFORESET			0x00000002
#define FIFOCONTROL_TXFIFORESET			0x00000004
#define FIFOCONTROL_RXFIFOTRIGGERLEVEL		0x000000C0

#define FIFOCONTROL_FIFOENABLE_SHIFT		(0)
#define FIFOCONTROL_RXFIFORESET_SHIFT		(1)
#define FIFOCONTROL_TXFIFORESET_SHIFT		(2)
#define FIFOCONTROL_RXFIFOTRIGGERLEVEL_SHIFT	(6)

#define FIFOCONTROL_FIFOENABLE_RANGE		0x00000001
#define FIFOCONTROL_RXFIFORESET_RANGE		0x00000001
#define FIFOCONTROL_TXFIFORESET_RANGE		0x00000001
#define FIFOCONTROL_RXFIFOTRIGGERLEVEL_RANGE	0x00000003

#define LINECONTROL_WORDLENGTHSELECT		0x00000003
#define LINECONTROL_STOPBITSSELECT		0x00000004
#define LINECONTROL_PARITYENABLE		0x00000008
#define LINECONTROL_PARITYSELECT		0x00000030
#define LINECONTROL_TRANSMITBREAK		0x00000040
#define LINECONTROL_DIVISORLATCHACCESS		0x00000080

#define LINECONTROL_WORDLENGTHSELECT_SHIFT	(0)
#define LINECONTROL_STOPBITSSELECT_SHIFT	(2)
#define LINECONTROL_PARITYENABLE_SHIFT		(3)
#define LINECONTROL_PARITYSELECT_SHIFT		(4)
#define LINECONTROL_TRANSMITBREAK_SHIFT		(6)
#define LINECONTROL_DIVISORLATCHACCESS_SHIFT	(7)

#define LINECONTROL_WORDLENGTHSELECT_RANGE	0x00000003
#define LINECONTROL_STOPBITSSELECT_RANGE	0x00000001
#define LINECONTROL_PARITYENABLE_RANGE		0x00000001
#define LINECONTROL_PARITYSELECT_RANGE		0x00000003
#define LINECONTROL_TRANSMITBREAK_RANGE		0x00000001
#define LINECONTROL_DIVISORLATCHACCESS_RANGE	0x00000001

#define MODEMCONTROL_DTRPIN			0x00000001
#define MODEMCONTROL_RTSPIN			0x00000002
#define MODEMCONTROL_OUT1PIN			0x00000004
#define MODEMCONTROL_OUT2PIN			0x00000008
#define MODEMCONTROL_LOOPBACKENABLE		0x00000010

#define MODEMCONTROL_DTRPIN_SHIFT		(0)
#define MODEMCONTROL_RTSPIN_SHIFT		(1)
#define MODEMCONTROL_OUT1PIN_SHIFT		(2)
#define MODEMCONTROL_OUT2PIN_SHIFT		(3)
#define MODEMCONTROL_LOOPBACKENABLE_SHIFT	(4)

#define MODEMCONTROL_DTRPIN_RANGE		0x00000001
#define MODEMCONTROL_RTSPIN_RANGE		0x00000001
#define MODEMCONTROL_OUT1PIN_RANGE		0x00000001
#define MODEMCONTROL_OUT2PIN_RANGE		0x00000001
#define MODEMCONTROL_LOOPBACKENABLE_RANGE	0x00000001

#define LINESTATUS_DATAREADY			0x00000001
#define LINESTATUS_OVERRUNERROR			0x00000002
#define LINESTATUS_PARITYERROR			0x00000004
#define LINESTATUS_FAMINGERROR			0x00000008
#define LINESTATUS_BREAKSIGNAL			0x00000010
#define LINESTATUS_TRANSMITTERHODINGEMPTY	0x00000020
#define LINESTATUS_TRANSMITTEREMPTY		0x00000040
#define LINESTATUS_ERRORINRXFIFO		0x00000080

#define LINESTATUS_DATAREADY_SHIFT		(0)
#define LINESTATUS_OVERRUNERROR_SHIFT		(1)
#define LINESTATUS_PARITYERROR_SHIFT		(2)
#define LINESTATUS_FAMINGERROR_SHIFT		(3)
#define LINESTATUS_BREAKSIGNAL_SHIFT		(4)
#define LINESTATUS_TRANSMITTERHODINGEMPTY_SHIFT	(5)
#define LINESTATUS_TRANSMITTEREMPTY_SHIFT	(6)
#define LINESTATUS_ERRORINRXFIFO_SHIFT		(7)

#define LINESTATUS_DATAREADY_RANGE		0x00000001
#define LINESTATUS_OVERRUNERROR_RANGE		0x00000001
#define LINESTATUS_PARITYERROR_RANGE		0x00000001
#define LINESTATUS_FAMINGERROR_RANGE		0x00000001
#define LINESTATUS_BREAKSIGNAL_RANGE		0x00000001
#define LINESTATUS_TRANSMITTERHODINGEMPTY_RANGE	0x00000001
#define LINESTATUS_TRANSMITTEREMPTY_RANGE	0x00000001
#define LINESTATUS_ERRORINRXFIFO_RANGE		0x00000001

#define MODEMSTATUS_CTSCHANGE		0x00000001
#define MODEMSTATUS_DSRCHANGE		0x00000002
#define MODEMSTATUS_RICHANGE		0x00000004
#define MODEMSTATUS_DCDCHANGE		0x00000008
#define MODEMSTATUS_CTS			0x00000010
#define MODEMSTATUS_DSR			0x00000020
#define MODEMSTATUS_RI			0x00000040
#define MODEMSTATUS_DCD			0x00000080

#define MODEMSTATUS_CTSCHANGE_SHIFT	(0)
#define MODEMSTATUS_DSRCHANGE_SHIFT	(1)
#define MODEMSTATUS_RICHANGE_SHIFT	(2)
#define MODEMSTATUS_DCDCHANGE_SHIFT	(3)
#define MODEMSTATUS_CTS_SHIFT		(4)
#define MODEMSTATUS_DSR_SHIFT		(5)
#define MODEMSTATUS_RI_SHIFT		(6)
#define MODEMSTATUS_DCD_SHIFT		(7)

#define MODEMSTATUS_CTSCHANGE_RANGE	0x00000001
#define MODEMSTATUS_DSRCHANGE_RANGE	0x00000001
#define MODEMSTATUS_RICHANGE_RANGE	0x00000001
#define MODEMSTATUS_DCDCHANGE_RANGE	0x00000001
#define MODEMSTATUS_CTS_RANGE		0x00000001
#define MODEMSTATUS_DSR_RANGE		0x00000001
#define MODEMSTATUS_RI_RANGE		0x00000001
#define MODEMSTATUS_DCD_RANGE		0x00000001

/*
 * ----
 * layer hardware driver
 */
/* lz_ layer zero */

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
	struct uart_port	suartport;
	    /* container_of(adapter, struct module_port_device, si2cadapter); */
	char			*coredev_name;
		/* dev_name(ThisCoreDevice.dev) */

	/* device capability */
	unsigned int		dsmcheck;
	unsigned int		ahc0502rev;

	/* monitor device status */

	/* device handles and resources */
	u32	porttype;
	u32	uartspeedmode;
	u32	clockrate;
	u32	clockdiv;
	u32	minbaudratedivisor;

	u32	uartconfigtype;
	u32	tx_loadsz;
	u32	fifo_size;

	u32	last_ier;
	u32	last_lcr;
	u32	last_mcr;
	u32 baseuartclock;					 

	struct list_head	shared_irq_entry;
		/* shared_irq_entry will link all IRQs this module used.*/

	/*struct timer_list	timer_noirq;*/

	struct tasklet_struct	tasklet_isrdpc;

	/* Synchronization */

	/* ISRDPC Queue and Rountine */
	spinlock_t		interruptlock_irq;
		/* for some operation that will generate interrupt */

	spinlock_t		interruptlock_dpcqueue;
	/*struct interrupt_extension sinterrupt_extension;*/
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

#define MAX_MODULE_PORTS	16
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

/*static DEVICE_ATTR_RO(drivername);*/
	/* dev_attr_drivername .show = drivername_show, .store = null*/
	/* DEVICE_ATTR_RO DEVICE_ATTR_WO DEVICE_ATTR_RW */
	/* default func name is drivername_show drivername_store */
	/* static DEVICE_ATTR(registers,..); dev_attr_registers */
	/* static DEVICE_ATTR(function,..); dev_attr_function */
	/* static DEVICE_ATTR(queues,..); dev_attr_queues */

#define THISDEVICEFILE_MAJOR 30
#define THISDEVICEFILE_MINOR 0

static struct uart_driver thismodule_uart_driver = {
	.owner		= THIS_MODULE,
	.driver_name	= DRIVER_NAME,
	.dev_name	= "ttyAHC",
	.major		= THISDEVICEFILE_MAJOR,
	.minor		= THISDEVICEFILE_MINOR,
	.nr		= MAX_MODULE_PORTS,
	.cons		= NULL,
};

#define PORTTYPE_UNKNOWN	0
#define PORTTYPE_RS232		1
#define PORTTYPE_RS422		2
#define PORTTYPE_RS485		3

#define EUART_UNKNOWN		0
#define EUART_232_V1		1
#define EUART_422485_V1		2
#define EUART_A9610_232_V1	3
#define EUART_A9610_422485_V1	4
struct uart_config {
	const char *name;
	u32	fifo_size;
	u32	tx_loadsz;
	u32	capabilities;
};

static const struct uart_config thismodule_uart_config[] = {
	[EUART_UNKNOWN] = {
		.name = "unknown uart",
		.fifo_size	= 1,
		.tx_loadsz	= 1,
	},
	[EUART_232_V1] = {
		.name = "EUART_232_V1",
		.fifo_size = 64,
		.tx_loadsz = 14,
	},
	[EUART_422485_V1] = {
		.name = "EUART_422485_V1",
		.fifo_size = 64,
		.tx_loadsz = 14,
	},
	[EUART_A9610_232_V1] = {
		.name = "EUART_A9610_232_V1",
		.fifo_size = 64,
		.tx_loadsz = 14,
	},
	[EUART_A9610_422485_V1] = {
		.name = "EUART_A9610_422485_V1",
		.fifo_size = 64,
		.tx_loadsz = 14,
	},
};

static int uart_ops_request_port(
	struct uart_port *uartport);
static void uart_ops_release_port(
	struct uart_port *uartport);
static void uart_ops_config_port(
	struct uart_port *uartport,
	int flags);
static const char *uart_ops_type(
	struct uart_port *uartport);
static int uart_ops_startup(
	struct uart_port *uartport);
static void uart_ops_shutdown(
	struct uart_port *uartport);
static void uart_ops_pm(
	struct uart_port *uartport,
	unsigned int state,
	unsigned int oldstate);
static void uart_ops_set_termios(
	struct uart_port *uartport,
	struct ktermios *termios,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))	
	const struct ktermios *oldtermios);
#else
	struct ktermios *oldtermios);
#endif	
static void uart_ops_set_mctrl(
	struct uart_port *uartport,
	unsigned int mctrl);
static unsigned int uart_ops_get_mctrl(
	struct uart_port *uartport);
static unsigned int uart_ops_tx_empty(
	struct uart_port *uartport);
static void uart_ops_start_tx(
	struct uart_port *uartport);
static void uart_ops_stop_tx(
	struct uart_port *uartport);
static void uart_ops_stop_rx(
	struct uart_port *uartport);
static void uart_ops_enable_ms(
	struct uart_port *uartport);
static void uart_ops_break_ctl(
	struct uart_port *uartport,
	int break_state);

static const struct uart_ops thismodule_uart_ops = {
	.release_port	= uart_ops_release_port,
	.request_port	= uart_ops_request_port,
	.config_port	= uart_ops_config_port,
	.type		= uart_ops_type,

	.startup	= uart_ops_startup,
	.shutdown	= uart_ops_shutdown,
	.pm		= uart_ops_pm,

	.set_termios	= uart_ops_set_termios,
	.set_mctrl	= uart_ops_set_mctrl,
	.get_mctrl	= uart_ops_get_mctrl,
	.tx_empty	= uart_ops_tx_empty,
	.start_tx	= uart_ops_start_tx,
	.stop_tx	= uart_ops_stop_tx,
	.stop_rx	= uart_ops_stop_rx,
	.enable_ms	= uart_ops_enable_ms,
	.break_ctl	= uart_ops_break_ctl,
	/*.ioctl		= uart_ops_ioctl,*/
};

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

	retval = uart_register_driver(&thismodule_uart_driver);
	if (retval < 0)
		pr_info("uart_register_driver failed\n");

	return retval;
}

static void moduleport_exit(void)
{
	pr_info("%s:\n", __func__);
	uart_unregister_driver(&thismodule_uart_driver);
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
	/*this_port_device->i2cadapter = NULL;*/

	this_port_device->dsmcheck = 0;
	this_port_device->porttype = 1;
	this_port_device->uartspeedmode = 6;
	this_port_device->last_ier = 0;
	this_port_device->last_lcr = 0;
	this_port_device->last_mcr = 0;

	tasklet_init(&this_port_device->tasklet_isrdpc, routine_isrdpc,
		(unsigned long)moduleportindex);
	spin_lock_init(&this_port_device->interruptlock_irq);
	spin_lock_init(&this_port_device->interruptlock_dpcqueue);
	/*memset(&this_port_device->sinterrupt_extension, 0,*/
	/*	sizeof(struct interrupt_extension));*/

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
	unsigned int uartconfigtype,
	unsigned int porttype,
	unsigned int ahc0502rev,
	unsigned int uartspeedmode)
{
	int retval; /* return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	struct module_port_device *this_port_device;

	pr_info("%s Entry moduleportindex:%u\n", __func__, moduleportindex);
	retval = 0;
	this_port_device = &thismodule_port_devices[moduleportindex];

	this_port_device->uartconfigtype = uartconfigtype;
	this_port_device->tx_loadsz =
		thismodule_uart_config[uartconfigtype].tx_loadsz;
	this_port_device->fifo_size =
		thismodule_uart_config[uartconfigtype].fifo_size;
	this_port_device->porttype = porttype;
	this_port_device->ahc0502rev = ahc0502rev;
	this_port_device->uartspeedmode = uartspeedmode;
	switch (uartspeedmode) {
	case 1:
		this_port_device->clockrate = 24000000;
		this_port_device->clockdiv = 16;
		this_port_device->minbaudratedivisor = 1;
		this_port_device->baseuartclock = 1500000*16;	//this value is the baud rate when div=1																									   
		break;
	case 4:
		this_port_device->clockrate = 1846153;
		this_port_device->clockdiv = 8;
		this_port_device->minbaudratedivisor = 1;
		this_port_device->baseuartclock = 230400*16;	//this value is the baud rate when div=1																									 
		break;
	case 5:
		this_port_device->clockrate = 24000000;
		this_port_device->clockdiv = 13;
		this_port_device->minbaudratedivisor = 1;
		this_port_device->baseuartclock = 1843200*16;	//this value is the baud rate when div=1																						   
		break;
	case 6:
		this_port_device->clockrate = 50000000;
		this_port_device->clockdiv = 1;
		this_port_device->minbaudratedivisor = 5;
		this_port_device->baseuartclock = 10000000*16;	//this value is the baud rate when div=5																							
		break;
	default:
		this_port_device->clockrate = 1846153;
		this_port_device->clockdiv = 16;
		this_port_device->minbaudratedivisor = 1;
		this_port_device->baseuartclock = 115200*16;	//this value is the baud rate when div=1																								   
		break;
	}

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
	/*int reterr; check if (reterr < 0) */
	unsigned int result; /* check if (result != 0) */

	struct module_port_device *this_port_device;
	struct uart_port *uartport;

	pr_info("%s Entry moduleportindex:%u\n", __func__, moduleportindex);
	retval = 0;

	pr_info("    dev_name %s\n", dev_name(dev)); /* ex: 0000:03:06.0 */

	/* pcidev = container_of(dev, struct pci_dev, device); */
	this_port_device = &thismodule_port_devices[moduleportindex];
	/* this_driver_data = dev_get_drvdata(dev); */
	/* this_specific = this_driver_data->this_specific; */

	/* allocate ThisCoreDevice */

	/* init ThisCoreDevice */
	uartport = &this_port_device->suartport;
	memset(uartport, 0, sizeof(struct uart_port));
	uartport->flags = UPF_SKIP_TEST | UPF_BOOT_AUTOCONF | UPF_SHARE_IRQ;
	uartport->uartclk = this_port_device->baseuartclock;
	uartport->irq = this_port_device->irq;
	uartport->dev = dev;/*parent device*/
	if (this_port_device->iospace) {
		uartport->iotype = UPIO_PORT;
		uartport->iobase = this_port_device->baseaddress;
		uartport->mapbase = 0;
		uartport->membase = NULL;
		uartport->regshift = 0;
	} else {
		uartport->iotype = UPIO_MEM;
		uartport->iobase = 0;
		uartport->mapbase = this_port_device->baseaddress;
		uartport->membase = this_port_device->reg_base;
		uartport->regshift = 0;
	}
	uartport->line = moduleportindex;/*port index*/
	spin_lock_init(&uartport->lock);
	uartport->ops = &thismodule_uart_ops;

	/* register ThisCoreDevice */
	result = uart_add_one_port(&thismodule_uart_driver, uartport);
	if (result != 0) {
		pr_err("    uart_add_one_port failed %i\n", result);
		retval = -EFAULT;
		goto funcexit;
	}
	/* now, register is ok */
	this_port_device->coredev_name = (char *)uartport->name;
		/*(char *)dev_name(&this_port_device->i2cadapter->dev);*/
	pr_info("    register ThisCoreDevice ok, dev_name:%s\n",
		this_port_device->coredev_name);
	if (thismodule_uart_driver.major) {
		this_port_device->major = thismodule_uart_driver.major;
		this_port_device->deviceid = uartport->minor;
		pr_info("    now, this port is registered with major:%u deviceid:%u\n",
			this_port_device->major, this_port_device->deviceid);
	} else {
		this_port_device->major = 0;
		this_port_device->deviceid = moduleportindex;
		pr_info("    this port have no major, so we let deviceid equal to moduleportindex:%u\n",
			this_port_device->deviceid);
	}

	/* create device attributes for ThisCoreDevice */
	/*reterr = device_create_file(
	 *	&this_i2c_adapter->dev, &dev_attr_drivername);
	 *if (reterr < 0) {
	 *	pr_err("    device_create_file failed %i\n", reterr);
	 *	retval = -EFAULT;
	 *	goto funcexit;
	 *}
	 */
	/* reterr = device_create_file(dev_attr_others); */

	goto funcexit;
funcexit:

	if (retval != 0)
		this_port_device->suartport.dev = NULL;

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

	if (this_port_device->suartport.dev != NULL) {
		/*device_remove_file(&this_port_device->i2cadapter->dev,
		 *	&dev_attr_drivername);
		 */

		/* unregister ThisCoreDevice */
		uart_remove_one_port(
			&thismodule_uart_driver, &this_port_device->suartport);

		this_port_device->suartport.dev = NULL;
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

	if (this_port_device->opencount == 1) {
		/* restart device */
		;
	}

	goto funcexit;
funcexit:

	pr_info("%s Exit\n", __func__);
	return retval;
}

static int port_startup(
	struct module_port_device *this_port_device)
{
	int retval; /* return -ERRNO */
	/*int reterr;check if (reterr < 0)*/
	/* unsigned int result; check if (result != 0) */

	retval = 0;

	/* EvtDevicePrepareHardware */
	/* uart ops will contorl startup steps*/

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

	/* EvtDeviceReleaseHardware */
	/* uart ops will contorl shutdown steps*/

}

static void port_pm(
	struct module_port_device *this_port_device,
	unsigned int state,
	unsigned int oldstate)
{
	/* int retval; return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	/* uart ops will contorl pm steps*/
}

static u32 uart_serial_in(
	struct uart_port *uartport,
	u32 offset)
{
	offset <<= uartport->regshift;
	if (uartport->iotype == SERIAL_IO_MEM)
		return readb(uartport->membase + offset);
	else
		return inb(uartport->iobase + offset);
}

static void uart_serial_out(
	struct uart_port *uartport,
	u32 offset,
	u32 value)
{
	offset <<= uartport->regshift;
	if (uartport->iotype == SERIAL_IO_MEM)
		writeb(value, uartport->membase + offset);
	else
		outb(value, uartport->iobase + offset);
}

static void receive_chars(
	struct uart_port *uartport,
	u32 *status)
{
	u8 ch;
	u8 lsr;
	u8 flagchar;
	s32 max_count = 256;

	lsr = *status;
	do {
		ch = uart_serial_in(uartport, UART_RX);
		uartport->icount.rx++;
		flagchar = TTY_NORMAL;
		if (unlikely(lsr & UART_LSR_BRK_ERROR_BITS)) {
			/*For statistics only*/
			if (lsr & UART_LSR_BI) {
				lsr &= ~(UART_LSR_FE | UART_LSR_PE);
				uartport->icount.brk++;
				/* We do the SysRQ and SAK checking
				 * here because otherwise the break
				 * may get masked by ignore_status_mask
				 * or read_status_mask.
				 */
				if (uart_handle_break(uartport))
					goto ignore_char;
			} else if (lsr & UART_LSR_PE)
				uartport->icount.parity++;
			else if (lsr & UART_LSR_FE)
				uartport->icount.frame++;
			if (lsr & UART_LSR_OE)
				uartport->icount.overrun++;
			/*Mask off conditions which should be ignored.*/
			lsr &= uartport->read_status_mask;
			if (lsr & UART_LSR_BI)
				flagchar = TTY_BREAK;
			else if (lsr & UART_LSR_PE)
				flagchar = TTY_PARITY;
			else if (lsr & UART_LSR_FE)
				flagchar = TTY_FRAME;
		}
		if (uart_handle_sysrq_char(uartport, ch))
			goto ignore_char;
		uart_insert_char(uartport, lsr, UART_LSR_OE, ch, flagchar);
ignore_char:
		lsr = uart_serial_in(uartport, UART_LSR);
	} while ((lsr & (UART_LSR_DR)) && (max_count-- > 0));

	spin_unlock(&uartport->lock);
	tty_flip_buffer_push(&uartport->state->port);
	spin_lock(&uartport->lock);

	*status = lsr;
}

static void transmit_chars(
	struct uart_port *uartport)
{
	struct module_port_device *this_port_device;
	struct circ_buf *xmit;
	s32 count;

	this_port_device =
		container_of(uartport, struct module_port_device, suartport);
	xmit = &uartport->state->xmit;
	/*send one xon or xoff*/
	if (uartport->x_char) {
		uart_serial_out(uartport, UART_TX, uartport->x_char);
		uartport->icount.tx++;
		uartport->x_char = 0;
		return;
	}

	if (uart_tx_stopped(uartport)) {
		uart_ops_stop_tx(uartport);
		return;
	}
	if (uart_circ_empty(xmit)) {
		uart_ops_stop_tx(uartport);
		return;
	}
	count = this_port_device->tx_loadsz;
	do {
		uart_serial_out(uartport, UART_TX, xmit->buf[xmit->tail]);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		uartport->icount.tx++;
		if (uart_circ_empty(xmit))
			break;
		count -= 1;
	} while (count > 0);
	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(uartport);
	if (uart_circ_empty(xmit))
		uart_ops_stop_tx(uartport);
}

static u32 check_modem_status(
	struct uart_port *uartport)
{
	u8 msr;

	msr = uart_serial_in(uartport, UART_MSR);
	if ((msr & UART_MSR_ANY_DELTA) == 0)
		return msr;
	if (msr & UART_MSR_TERI)
		uartport->icount.rng++;
	if (msr & UART_MSR_DDSR)
		uartport->icount.dsr++;
	if (msr & UART_MSR_DDCD)
		uart_handle_dcd_change(uartport, msr & UART_MSR_DCD);
	if (msr & UART_MSR_DCTS)
		uart_handle_cts_change(uartport, msr & UART_MSR_CTS);
	wake_up_interruptible(&uartport->state->port.delta_msr_wait);
	return msr;
}

static void uart_handle_port(
	struct uart_port *uartport)
{
	u32 lsr;

	lsr = uart_serial_in(uartport, UART_LSR);
	if (lsr & UART_LSR_DR)
		receive_chars(uartport, &lsr);
	check_modem_status(uartport);
	if (lsr & UART_LSR_THRE)
		transmit_chars(uartport);
}

static void uart_clear_port(
	struct uart_port *uartport)
{
	u8 lsr;
	u8 ch;

	lsr = uart_serial_in(uartport, UART_LSR);
	do {
		ch = uart_serial_in(uartport, UART_RX);
		lsr = uart_serial_in(uartport, UART_LSR);
	} while (lsr & (UART_LSR_DR));
	uart_serial_in(uartport, UART_MSR);
}

static irqreturn_t IRQHandler(int irq, void *devicedata)
{
	int retval; /* return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	struct shared_irq_list		*this_shared_irq_list;
	struct module_port_device	*this_port_device;
	struct uart_port		*uartport;
	u8 iir;

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

		uartport = &this_port_device->suartport;
		spin_lock(&uartport->lock);
		if (this_port_device->opencount == 0) {
			iir = uart_serial_in(uartport, UART_IIR);
			if (!(iir & UART_IIR_NO_INT))	{
				uart_clear_port(uartport);
				retval = IRQ_HANDLED;
			}
		} else {
			iir = uart_serial_in(uartport, UART_IIR);
			if (!(iir & UART_IIR_NO_INT))	{
				uart_handle_port(uartport);
				retval = IRQ_HANDLED;
			}
		}
		spin_unlock(&uartport->lock);
	}
	spin_unlock(&this_shared_irq_list->lock);

	goto funcexit;
funcexit:

	return retval;
}

static void routine_isrdpc(unsigned long moduleportindex)
{
}

static int uart_ops_request_port(
	struct uart_port *uartport)
{
	/*Request any memory and IO region resources required by the port*/
	/*we has done this at the process of pci_default_setup and
	 *pcidevice_probe
	 */
	return 0;
}

static void uart_ops_release_port(
	struct uart_port *uartport)

{
	/*Release any memory and IO region resources
	 *currently in use by the port
	 */
}

static void uart_ops_config_port(
	struct uart_port *uartport,
	int flags)
{
	struct module_port_device *this_port_device;
	u32	type;

	this_port_device =
		container_of(uartport, struct module_port_device, suartport);
	/*pr_info("%s Entry\n", __func__);
	 *pr_info("    portnumber:%u minor:%i\n",
	 *	this_port_device->portnumber, this_port_device->deviceid);
	 */
	if (flags & UART_CONFIG_TYPE) {
		type = this_port_device->uartconfigtype;
		uartport->type = type;
		uartport->fifosize = thismodule_uart_config[type].fifo_size;
		this_port_device->tx_loadsz =
			thismodule_uart_config[type].tx_loadsz;
		this_port_device->fifo_size =
			thismodule_uart_config[type].fifo_size;
		/*this_port_device->capabilities =
		 *	thismodule_uart_config[type].capabilities;
		 */
		/* reset the uart*/
			/* uart_ops_startup will do this.*/
		uart_serial_out(uartport, UART_MCR, 0);
		uart_serial_out(uartport, UART_LCR, 0);
		uart_serial_out(uartport, UART_IER, 0);
		uart_serial_out(uartport, UART_FCR, 0);
		/*clear interrupt*/
		uart_serial_in(uartport, UART_LSR);
		uart_serial_in(uartport, UART_RX);
		uart_serial_in(uartport, UART_IIR);
		uart_serial_in(uartport, UART_MSR);
		pr_info("    %s: UART_CONFIG_TYPE\n", __func__);
		pr_info("    %s: uartconfigtype:%u\n", __func__, type);
		pr_info("    %s: tx_loadsz:%u\n", __func__,
			this_port_device->tx_loadsz);
		pr_info("    %s: fifosize:%u\n", __func__,
			this_port_device->fifo_size);
	}

	if (flags & UART_CONFIG_IRQ) {
		pr_info("    %s: UART_CONFIG_IRQ %i\n", __func__,
			uartport->irq);
		/*we has done this at the process of pcidevice_probe*/
		/*uartport->irq = pcidev->irq;*/
		/*reset uart interrupt*/
		/*uart_ops_startup will do this*/
	}
}

static const char *uart_ops_type(
	struct uart_port *uartport)
{
	struct module_port_device *this_port_device;

	this_port_device =
		container_of(uartport, struct module_port_device, suartport);
	/*pr_info("%s Entry\n", __func__);
	 *pr_info("    portnumber:%u minor:%i\n",
	 *	this_port_device->portnumber, this_port_device->deviceid);
	 */

	/*maybe, we can use this info to
	 *show this port belog to which pci card.
	 */
	return thismodule_uart_config[uartport->type].name;
}

static int uart_ops_startup(
	struct uart_port *uartport)
/*at startup, serial core think that ModemStatus Interrupt is disable.*/
/*after pcidevice_probe, OS kernel call this function.*/
{
	int retval;
	struct module_port_device *this_port_device;
	unsigned long flags;/*irg flags*/
	u32 lsr;
	/*struct shared_irq_list *this_shared_irq_list;*/

	retval = 0;
	this_port_device =
		container_of(uartport, struct module_port_device, suartport);
	/*pr_info("%s Entry\n", __func__);
	 *pr_info("    portnumber:%u minor:%i\n",
	 *	this_port_device->portnumber, this_port_device->deviceid);
	 */

	/*configure uart_port_device*/
	this_port_device->last_ier = 0;
	this_port_device->last_lcr = 0;
	this_port_device->last_mcr = 0;
	/*hardware reset for uart port
	 *Clear the FIFO buffers and disable them.
	 *they will be reenabled in set_termios()
	 */
	uart_serial_out(uartport, UART_MCR, 0);
	uart_serial_out(uartport, UART_LCR, 0);
	uart_serial_out(uartport, UART_IER, 0);

	/*reference serial8250_clear_fifos(up);*/
	uart_serial_out(uartport, UART_FCR, UART_FCR_ENABLE_FIFO);
	uart_serial_out(uartport, UART_FCR, UART_FCR_ENABLE_FIFO |
		UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT);
	uart_serial_out(uartport, UART_FCR, 0);

	/*clear interrupt.*/
	uart_serial_in(uartport, UART_LSR);
	uart_serial_in(uartport, UART_RX);
	uart_serial_in(uartport, UART_IIR);
	uart_serial_in(uartport, UART_MSR);

	/* At this point, there's no way the LSR could still be 0xff;
	 * if it is, then bail out, because there's likely no UART here.
	 */
	lsr = uart_serial_in(uartport, UART_LSR);
	if (lsr == 0xFF) {
		pr_info("    %s%i: LSR safety check engaged!\n",
			thismodule_uart_driver.dev_name,
			this_port_device->deviceid);
		retval = -ENODEV;
		goto funcexit;
	}

	this_port_device->opencount = 1;
	/*link shared_irq_entry to shared_irq_list*/
	/*INIT_LIST_HEAD(&this_port_device->shared_irq_entry);
	 *if (is_real_interrupt(this_port_device->irq)) {
	 *	this_shared_irq_list =
	 *		&thismodule_shared_irq_lists[this_port_device->irq];
	 *	spin_lock_irq(&this_shared_irq_list->lock);
	 *	list_add_tail(&this_port_device->shared_irq_entry,
	 *		&this_shared_irq_list->head);
	 *	spin_unlock_irq(&this_shared_irq_list->lock);
	 *	pr_info("    link shared_irq_entry to shared_irq_list\n");
	 *}
	 */

	/*Now, initialize the UART*/
	this_port_device->last_lcr = UART_LCR_WLEN8;
	uart_serial_out(uartport, UART_LCR, this_port_device->last_lcr);

	spin_lock_irqsave(&uartport->lock, flags);
	/*Most PC uarts need OUT2 raised to enable interrupts.*/
	/*if (is_real_interrupt(uartport->irq))
	 *	uartport->mctrl |= TIOCM_OUT2;
	 */
	uart_ops_set_mctrl(uartport, uartport->mctrl);
	spin_unlock_irqrestore(&uartport->lock, flags);

	/*Finally, enable interrupts.*/
	/*Note: Modem status interrupts are set via set_termios()*/
	this_port_device->last_ier = UART_IER_RLSI | UART_IER_RDI;
	uart_serial_out(uartport, UART_IER, this_port_device->last_ier);
	/*clear interrupt again.*/
	uart_serial_in(uartport, UART_LSR);
	uart_serial_in(uartport, UART_RX);
	uart_serial_in(uartport, UART_IIR);
	uart_serial_in(uartport, UART_MSR);

	goto funcexit;
funcexit:
	return retval;
}

static void uart_ops_shutdown(
	struct uart_port *uartport)
{
	struct module_port_device *this_port_device;
	unsigned long flags;/*irg flags*/
	/*struct shared_irq_list *this_shared_irq_list;*/

	this_port_device =
		container_of(uartport, struct module_port_device, suartport);
	/*pr_info("%s Entry\n", __func__);
	 *pr_info("    portnumber:%u minor:%i\n",
	 *	this_port_device->portnumber, this_port_device->deviceid);
	 */

	/*Disable interrupts from this port*/
	this_port_device->last_ier = 0;
	uart_serial_out(uartport, UART_IER, this_port_device->last_ier);
	spin_lock_irqsave(&uartport->lock, flags);
	uartport->mctrl &= ~TIOCM_OUT2;
	uart_ops_set_mctrl(uartport, uartport->mctrl);
	spin_unlock_irqrestore(&uartport->lock, flags);

	/*Disable break condition and FIFOs*/
	uart_serial_out(uartport, UART_LCR,
		uart_serial_in(uartport, UART_LCR) & ~UART_LCR_SBC);
	uart_serial_out(uartport, UART_FCR, UART_FCR_ENABLE_FIFO |
		UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT);
	uart_serial_out(uartport, UART_FCR, 0);
	/*Read data port to reset things, and then unlink from the IRQ chain.*/
	uart_serial_in(uartport, UART_RX);
	/*unlink shared_irq_entry from shared_irq_list*/
	this_port_device->opencount = 0;
}

static void uart_ops_pm(
	struct uart_port *uartport,
	unsigned int state,
	unsigned int oldstate)
/*if we supply port power, we can implement power control.*/
/*after uart_ops_startup, port power set to D0.*/
/* after uart_ops_shutdown, port power set to D3.*/
{
	struct module_port_device *this_port_device;

	this_port_device =
		container_of(uartport, struct module_port_device, suartport);
	/*pr_info("%s Entry\n", __func__);
	 *pr_info("    portnumber:%u minor:%i\n",
	 *	this_port_device->portnumber, this_port_device->deviceid);
	 *pr_info("    state:D%u oldstate:D%u\n", state, oldstate);
	 */
	if (state != 0) {
		/*prepare port sleep*/
		/*keep uart config*/
		;
	} else {
		/*prepare port wakeup*/
		/*restore uart config*/
		;
	}
}

static void uart_ops_set_termios(
	struct uart_port *uartport,
	struct ktermios *termios,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))	
	const struct ktermios *oldtermios)
#else
	struct ktermios *oldtermios)
#endif	
{
	struct module_port_device *this_port_device;
	u32 lcr;
	u32 baud;
	u32 quot;
	unsigned long flags;

	this_port_device =
		container_of(uartport, struct module_port_device, suartport);
	/*pr_info("%s Entry\n", __func__);
	 *pr_info("    portnumber:%u minor:%i\n",
	 *	this_port_device->portnumber, this_port_device->deviceid);
	 *pr_info("    uart_ops_pm: state:D%u oldstate:D%u\n", state, oldstate);
	 */
	lcr = 0;
	baud = 0;
	quot = 0;

	switch (termios->c_cflag & CSIZE) {
	case CS5:
		lcr = UART_LCR_WLEN5;
		break;
	case CS6:
		lcr = UART_LCR_WLEN6;
		break;
	case CS7:
		lcr = UART_LCR_WLEN7;
		break;
	default:/*case CS8:*/
		lcr = UART_LCR_WLEN8;
		break;
	}
	if (termios->c_cflag & CSTOPB)
		lcr |= UART_LCR_STOP;
	if (termios->c_cflag & PARENB) {
		lcr |= UART_LCR_PARITY;
		if (termios->c_cflag & CMSPAR)
			lcr |= UART_LCR_SPAR;
		if (!(termios->c_cflag & PARODD))
			lcr |= UART_LCR_EPAR;
	}
	/*ask the core to calculate the divisor for us*/
	/*MaxBaudRate = (SourceClock / ClockDIV).*/
	baud = uart_get_baud_rate(uartport, termios, oldtermios, 0,
		uartport->uartclk / this_port_device->clockdiv);
	/*BRDIV = (SourceClock / ClockDIV) / BaudRate;*/
	quot = (this_port_device->clockrate / this_port_device->clockdiv) /
		baud;
	if (this_port_device->minbaudratedivisor != 0 &&
		quot < this_port_device->minbaudratedivisor)
		quot = this_port_device->minbaudratedivisor;
	pr_info("    %s: baud:%u quot:%u actual baud:%u\n", __func__,
		baud, quot, (this_port_device->clockrate /
		this_port_device->clockdiv) / quot);
	/*Ok, we're now changing the port state.
	 *Do it with interrupts disabled.
	 */
	spin_lock_irqsave(&uartport->lock, flags);
	/*Update the per-port timeout.*/
	uart_update_timeout(uartport, termios->c_cflag, baud);
	uartport->read_status_mask = UART_LSR_OE | UART_LSR_THRE | UART_LSR_DR;
	if (termios->c_iflag & INPCK)
		uartport->read_status_mask |= UART_LSR_FE | UART_LSR_PE;
	if (termios->c_iflag & (BRKINT | PARMRK))
		uartport->read_status_mask |= UART_LSR_BI;
	/*Characteres to ignore*/
	uartport->ignore_status_mask = 0;
	if (termios->c_iflag & IGNPAR)
		uartport->ignore_status_mask |= UART_LSR_PE | UART_LSR_FE;
	if (termios->c_iflag & IGNBRK) {
		uartport->ignore_status_mask |= UART_LSR_BI;
		/*If we're ignoring parity and break indicators,
		 *ignore overruns too (for real raw support).
		 */
		if (termios->c_iflag & IGNPAR)
			uartport->ignore_status_mask |= UART_LSR_OE;
	}
	/*ignore all characters if CREAD is not set*/
	if ((termios->c_cflag & CREAD) == 0)
		uartport->ignore_status_mask |= UART_LSR_DR;

	/*CTS flow control flag and modem status interrupts*/
	this_port_device->last_ier &= ~UART_IER_MSI;
	if (UART_ENABLE_MS(uartport, termios->c_cflag))
		this_port_device->last_ier |= UART_IER_MSI;
	uart_serial_out(uartport, UART_IER, this_port_device->last_ier);
	/*update lcr with enable DLAB*/
	uart_serial_out(uartport, UART_LCR, lcr | UART_LCR_DLAB);
	/*update divisor*/
	uart_serial_out(uartport, UART_DLL, quot & 0xff);
	uart_serial_out(uartport, UART_DLM, quot >> 8);
	/*update lcr with disable DLAB*/
	uart_serial_out(uartport, UART_LCR, lcr);
	/*Save LCR*/
	this_port_device->last_lcr = lcr;
	/*enable FIFO*/
	uart_serial_out(uartport, UART_FCR, UART_FCR_ENABLE_FIFO |
		UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT);
	uart_serial_out(uartport, UART_FCR, UART_FCR_TRIGGER_14 |
		UART_FCR_ENABLE_FIFO);
	spin_unlock_irqrestore(&uartport->lock, flags);
}

static void uart_ops_set_mctrl(
	struct uart_port *uartport,
	unsigned int mctrl)
{
	struct module_port_device *this_port_device;
	u32 mcr;

	this_port_device =
		container_of(uartport, struct module_port_device, suartport);
	/*pr_info("%s Entry\n", __func__);
	 *pr_info("    portnumber:%u minor:%i\n",
	 *	this_port_device->portnumber, this_port_device->deviceid);
	 */

	mcr = 0;
	if (mctrl & TIOCM_DTR)
		mcr |= UART_MCR_DTR;
	if (mctrl & TIOCM_RTS)
		mcr |= UART_MCR_RTS;
	if (mctrl & TIOCM_OUT1)
		mcr |= UART_MCR_OUT1;
	if (mctrl & TIOCM_OUT2)
		mcr |= UART_MCR_OUT2;
	if (mctrl & TIOCM_LOOP)
		mcr |= UART_MCR_LOOP;
	this_port_device->last_mcr = mcr;
	uart_serial_out(uartport, UART_MCR, this_port_device->last_mcr);
}


static unsigned int uart_ops_get_mctrl(
	struct uart_port *uartport)
	/*serial core use spin_lock_irq to protect this function.
	 *so we can not use spin_lock_irqsave(&uartport->lock, flags)
	 *in this fuction
	 */
{
	struct module_port_device *this_port_device;
	unsigned int retval;
	u32 msr;

	this_port_device =
		container_of(uartport, struct module_port_device, suartport);
	/*pr_info("%s Entry\n", __func__);
	 *pr_info("    portnumber:%u minor:%i\n",
	 *	this_port_device->portnumber, this_port_device->deviceid);
	 */

	retval = 0;
	msr = check_modem_status(uartport);
	if (msr & UART_MSR_CTS)
		retval |= TIOCM_CTS;
	if (msr & UART_MSR_DSR)
		retval |= TIOCM_DSR;
	if (msr & UART_MSR_RI)
		retval |= TIOCM_RNG;
	if (msr & UART_MSR_DCD)
		retval |= TIOCM_CAR;
	return retval;
}

static unsigned int uart_ops_tx_empty(
	struct uart_port *uartport)
{
	struct module_port_device *this_port_device;
	unsigned int retval;
	unsigned long flags;

	this_port_device =
		container_of(uartport, struct module_port_device, suartport);
	/*pr_info("%s Entry\n", __func__);
	 *pr_info("    portnumber:%u minor:%i\n",
	 *	this_port_device->portnumber, this_port_device->deviceid);
	 */

	retval = 0;
	spin_lock_irqsave(&uartport->lock, flags);
	retval = uart_serial_in(uartport, UART_LSR) & UART_LSR_TEMT ?
		TIOCSER_TEMT : 0;
	spin_unlock_irqrestore(&uartport->lock, flags);
	return retval;
}

static void uart_ops_start_tx(
	struct uart_port *uartport)
	/*serial core use spin_lock_irq to protect this function.*/
{
	struct module_port_device *this_port_device;

	this_port_device =
		container_of(uartport, struct module_port_device, suartport);
	/*pr_info("%s Entry\n", __func__);
	 *pr_info("    portnumber:%u minor:%i\n",
	 *	this_port_device->portnumber, this_port_device->deviceid);
	 */

	if (!(this_port_device->last_ier & UART_IER_THRI)) {
		this_port_device->last_ier |= UART_IER_THRI;
		uart_serial_out(uartport, UART_IER, this_port_device->last_ier);
	}
}

static void uart_ops_stop_tx(
	struct uart_port *uartport)
	/*serial core use spin_lock_irq to protect this function.*/
{
	struct module_port_device *this_port_device;

	this_port_device =
		container_of(uartport, struct module_port_device, suartport);
	/*pr_info("%s Entry\n", __func__);
	 *pr_info("    portnumber:%u minor:%i\n",
	 *	this_port_device->portnumber, this_port_device->deviceid);
	 */

	if (this_port_device->last_ier & UART_IER_THRI) {
		this_port_device->last_ier &= ~UART_IER_THRI;
		uart_serial_out(uartport, UART_IER, this_port_device->last_ier);
	}
}

static void uart_ops_stop_rx(
	struct uart_port *uartport)
	/*serial core use spin_lock_irq to protect this function.*/
	/*disable LineStatus Interrupt*/
	/*serial core use this function on uart_close() and uart_suspend()*/
{
	struct module_port_device *this_port_device;

	this_port_device =
		container_of(uartport, struct module_port_device, suartport);
	/*pr_info("%s Entry\n", __func__);
	 *pr_info("    portnumber:%u minor:%i\n",
	 *	this_port_device->portnumber, this_port_device->deviceid);
	 */

	uartport->read_status_mask &= ~UART_LSR_DR;
	this_port_device->last_ier &= ~UART_IER_RLSI;
	uart_serial_out(uartport, UART_IER, this_port_device->last_ier);
}

static void uart_ops_enable_ms(
	struct uart_port *uartport)
	/*enable ModemStatus Interrupt*/
	/*serial core use this function to enable ModemStatus Interrupt.*/
{
	struct module_port_device *this_port_device;

	this_port_device =
		container_of(uartport, struct module_port_device, suartport);
	/*pr_info("%s Entry\n", __func__);
	 *pr_info("    portnumber:%u minor:%i\n",
	 *	this_port_device->portnumber, this_port_device->deviceid);
	 */

	this_port_device->last_ier |= UART_IER_MSI;
	uart_serial_out(uartport, UART_IER, this_port_device->last_ier);
}

static void uart_ops_break_ctl(
	struct uart_port *uartport,
	int break_state)
{
	struct module_port_device *this_port_device;
	unsigned long flags;

	this_port_device =
		container_of(uartport, struct module_port_device, suartport);
	/*pr_info("%s Entry\n", __func__);
	 *pr_info("    portnumber:%u minor:%i\n",
	 *	this_port_device->portnumber, this_port_device->deviceid);
	 *pr_info("    break_state:%i\n", break_state);
	 */

	spin_lock_irqsave(&uartport->lock, flags);
	if (break_state == -1)
		this_port_device->last_lcr |= UART_LCR_SBC;
	else
		this_port_device->last_lcr &= ~UART_LCR_SBC;
	uart_serial_out(uartport, UART_LCR, this_port_device->last_lcr);
	spin_unlock_irqrestore(&uartport->lock, flags);
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
	unsigned int	uartconfigtype;
	unsigned int	porttype;
	unsigned int	uartspeedmode;
	unsigned int	num_parports;
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
MODULE_DESCRIPTION("RDC Platform UART Controller Driver");
MODULE_VERSION(DRIVER_VERSION);

/*
 * ----
 * define module parameters: mp_
 * if perm is 0, no sysfs entry.
 * if perm is S_IRUGO 0444, there is a sysfs entry, but cant altered.
 * if perm is S_IRUGO | S_IWUSR, there is a sysfs entry and can altered by root.
 */
/*static int mp_fastmode;static variable always zero*/
/*MODULE_PARM_DESC(mp_fastmode, " FastMode: 0 or 1 (default:0)");*/
/*module_param(mp_fastmode, int, 0444);*/

/*
 * ----
 * define multiple pnp devices for this module supports
 */
#define MAX_SPECIFIC	2 /* this module support */
enum platform_specific_num_t {
	platform_AHC0501 = 0, /* fixed on board */
	platform_AHC0502 = 1, /* fixed on board */
};

static int platform_default_init(
	struct platform_driver_data *this_driver_data);
static int platform_default_setup(
	struct platform_driver_data *this_driver_data);
static void platform_default_exit(
	struct platform_driver_data *this_driver_data);

static struct platform_specific thismodule_platform_specifics[] = {
	[platform_AHC0501] = {
		.vendor		= 0x17f3,
		.device		= 0x121C,
		.subvendor	= PCI_ANY_ID,
		.subdevice	= PCI_ANY_ID,

		.init		= platform_default_init,
		.setup		= platform_default_setup,
		.exit		= platform_default_exit,

		.pid		= 0xA9610,
		.uartconfigtype	= EUART_A9610_232_V1,
		.porttype	= PORTTYPE_RS232,
		.uartspeedmode	= 0,
		.num_parports	= 0,
		.basebar_forports = 0,
		.num_ports	= 1,
		.first_offset	= 0,
		.reg_offset	= 0,
		.reg_shift	= 0,
		.reg_length	= 8,
	},
	[platform_AHC0502] = {
		.vendor		= 0x17f3,
		.device		= 0x121C,
		.subvendor	= PCI_ANY_ID,
		.subdevice	= PCI_ANY_ID,

		.init		= platform_default_init,
		.setup		= platform_default_setup,
		.exit		= platform_default_exit,

		.pid		= 0xA9610,
		.uartconfigtype	= EUART_A9610_232_V1,
		.porttype	= PORTTYPE_RS232,
		.uartspeedmode	= 0,
		.num_parports	= 0,
		.basebar_forports = 0,
		.num_ports	= 1,
		.first_offset	= 0,
		.reg_offset	= 0,
		.reg_shift	= 0,
		.reg_length	= 8,
	},
};

/*
 * ----
 * define pnp device table for this module supports
 * PNP_ID_LEN = 8
 */
static const struct pnp_device_id pnp_device_id_table[] = {
	{ .id = "PNP0501", .driver_data = platform_AHC0501, },
	{ .id = "AHC0501", .driver_data = platform_AHC0501, },
	{ .id = "AHC0502", .driver_data = platform_AHC0502, },
	{ } /* terminate list */
};
MODULE_DEVICE_TABLE(pnp, pnp_device_id_table);

static const struct platform_device_id platform_device_id_table[] = {
	/* we take driver_data as a specific number(ID). */
	{ .name = "PNP0501", .driver_data = platform_AHC0501, },
	{ .name = "AHC0501", .driver_data = platform_AHC0501, },
	{ .name = "AHC0502", .driver_data = platform_AHC0502, },
	{ } /* terminate list */
};
MODULE_DEVICE_TABLE(platform, platform_device_id_table);

static const struct of_device_id of_device_id_table[] = {
	{ .compatible = "rdc,ahc-com", .data = platform_AHC0501, },
	{},
};
MODULE_DEVICE_TABLE(of, of_device_id_table);

#ifdef CONFIG_ACPI
static const struct acpi_device_id acpi_device_id_table[] = {
	{ "PNP0501", platform_AHC0501, },
	{ "AHC0501", platform_AHC0501, },
	{ "AHC0502", platform_AHC0502, },
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
#define str_guid_ahc_uart_dsm "04353598-1e8d-4cbc-928a-73512a95346c"

static u32 _dsm_check(
	acpi_handle handle)
{
	struct acpi_object_list input;
	union acpi_object mt_params[4];
	struct acpi_buffer output = { ACPI_ALLOCATE_BUFFER, NULL };
	union acpi_object *o_params;
	acpi_status status;
	unsigned int ahc0502rev;
	guid_t	tguid;

	guid_parse(str_guid_ahc_uart_dsm, &tguid);
	ahc0502rev = 0;
	input.count = 4;
	input.pointer = mt_params;
	mt_params[0].type = ACPI_TYPE_BUFFER;
	mt_params[0].buffer.length = UUID_SIZE;
	mt_params[0].buffer.pointer = (u8 *)&tguid;
	mt_params[1].type = ACPI_TYPE_INTEGER;
	mt_params[1].integer.value = 0;
	mt_params[2].type = ACPI_TYPE_INTEGER;
	mt_params[2].integer.value = 0;
	mt_params[3].type = ACPI_TYPE_PACKAGE;
	mt_params[3].package.count = 0;
	mt_params[3].package.elements = NULL;

	status = acpi_evaluate_object(handle, "_DSM", &input, &output);
	if (ACPI_FAILURE(status)) {
		pr_info("    %s failed to evaluate _DSM: %i\n",
			__func__, status);
		return 0;
	}
	if (!output.pointer)
		return 0;

	o_params = output.pointer;
	pr_info("...output type %u\n", o_params->type);
	if (o_params->type == ACPI_TYPE_INTEGER) {
		pr_info("...output integer.value %llx\n",
			o_params->integer.value);
		ahc0502rev = (u8)o_params->integer.value & 0x01;
	} else if (o_params->type == ACPI_TYPE_BUFFER) {
		pr_info("...output buffer.length %u buffer[0] %x\n",
			o_params->buffer.length, o_params->buffer.pointer[0]);
		ahc0502rev = (u8)o_params->buffer.pointer[0] & 0x01;
	}

	kfree(output.pointer);

	return ahc0502rev;
}

static int _dsm_portconfig(
	acpi_handle handle,
	unsigned int uartspeedmode)
{
	struct acpi_object_list input;
	union acpi_object mt_params[4];
	struct acpi_buffer output = { ACPI_ALLOCATE_BUFFER, NULL };
	union acpi_object *o_params;
	acpi_status status;
	unsigned int portconfig;
	guid_t	tguid;

	guid_parse(str_guid_ahc_uart_dsm, &tguid);
	portconfig = 0x20 | uartspeedmode;
	input.count = 4;
	input.pointer = mt_params;
	mt_params[0].type = ACPI_TYPE_BUFFER;
	mt_params[0].buffer.length = UUID_SIZE;
	mt_params[0].buffer.pointer = (char *)&tguid;
	mt_params[1].type = ACPI_TYPE_INTEGER;
	mt_params[1].integer.value = 0;
	mt_params[2].type = ACPI_TYPE_INTEGER;
	mt_params[2].integer.value = 1;
	mt_params[3].type = ACPI_TYPE_INTEGER;
	mt_params[3].integer.value = portconfig;

	status = acpi_evaluate_object(handle, "_DSM", &input, &output);
	if (ACPI_FAILURE(status)) {
		pr_info("    %s failed to evaluate _DSM: %i\n",
			__func__, status);
		return 0;
	}
	if (!output.pointer)
		return 0;

	o_params = output.pointer;
	pr_info("    output type %u\n", o_params->type);
	if (o_params->type == ACPI_TYPE_INTEGER) {
		pr_info("    output integer.value %llx\n",
			o_params->integer.value);
	} else if (o_params->type == ACPI_TYPE_BUFFER) {
		pr_info("...output buffer.length %u buffer[0] %x\n",
			o_params->buffer.length, o_params->buffer.pointer[0]);
	}

	kfree(output.pointer);
	return portconfig;
}

static int _basedevice_probe(
	unsigned int devtype,
	void *thisdev/*this_dev*/,
	struct platform_specific *this_specific,
	unsigned int specific_id)
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
	unsigned int ahc0502rev;
	unsigned int uartspeedmode;

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

		/*if ahc0502, then check dsm.*/
		/*if dsm, then uartspeedmode = 5 else uartspeedmode = 0.*/
		ahc0502rev = 0;
		uartspeedmode = this_specific->uartspeedmode;
		if (ACPI_HANDLE(_dev) && specific_id == 1)
			ahc0502rev = _dsm_check(ACPI_HANDLE(_dev));
		if (ahc0502rev == 1) {
			uartspeedmode = 6;
			_dsm_portconfig(ACPI_HANDLE(_dev), uartspeedmode);
		}

		module_setting_port_device(moduleportindex,
			this_specific->uartconfigtype, this_specific->porttype,
			ahc0502rev, uartspeedmode);

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
	struct module_port_device *this_port_device;
	struct device *_dev;/*base class*/
	struct platform_device	*platformdev;
	struct pnp_dev		*pnpdev;

	retval = 0;
	this_specific = this_driver_data->this_specific;
	_dev = NULL;
	switch (this_driver_data->devtype) {
	case DEVTYPE_PCIDEV:
		break;
	case DEVTYPE_PLATFORMDEV:
		platformdev = (struct platform_device *)
			this_driver_data->thisdev;
		_dev = &platformdev->dev;
		break;
	case DEVTYPE_PNPDEV:
		pnpdev = (struct pnp_dev *)this_driver_data->thisdev;
		_dev = &pnpdev->dev;
		break;
	}

	/* resume (adapter)device */

	/* resume each port device */
	for (i = 0; i < this_specific->num_ports; i++) {
		/*pr_info("port id: %u\n", i);*/
		moduleportindex = this_driver_data->module_port_index[i];
		if (moduleportindex >= 0)
			module_resume_thiscoredev(moduleportindex);
		this_port_device = &thismodule_port_devices[moduleportindex];
		if (this_port_device->ahc0502rev == 1)
			_dsm_portconfig(ACPI_HANDLE(_dev), 5);
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

	pr_info("%s Entry\n", __func__);
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
		this_specific, specific_id);
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
	struct acpi_device *acpi_dev;
	struct acpi_hardware_id *id;

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
	acpi_dev = ACPI_COMPANION(&pnpdev->dev);
	if (!acpi_dev) {
		pr_info("    ACPI device not found in %s!\n", __func__);
	} else {
		list_for_each_entry(id, &acpi_dev->pnp.ids, list) {
			if (!strcmp(id->id, pnp_device_id_table[2].id))
				specific_id = 1;
		}
		this_specific = &thismodule_platform_specifics[specific_id];
	}
	pr_info("    specific_id:%u\n", specific_id);

	retval = _basedevice_probe(DEVTYPE_PNPDEV, pnpdev,
		this_specific, specific_id);
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
	retval = module_shared_irq_reference(this_driver_data->irq);

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
	module_shared_irq_dereference(this_driver_data->irq);

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

