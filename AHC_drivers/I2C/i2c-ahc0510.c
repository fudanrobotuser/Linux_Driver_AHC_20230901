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
#include <linux/kdev_t.h> /* MAJOR, MINOR	*/

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
#include <linux/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

/* our device header */
/*
 * return -ERRNO -EFAULT -ENOMEM -EINVAL
 * device -EBUSY, -ENODEV, -ENXIO
 * parameter -EOPNOTSUPP
 * pr_info pr_warn pr_err
 */
#define DRIVER_NAME		"RDC I2C"
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

#define LZ_I2CREGOFFSET_CONTROL		0x00
#define LZ_I2CREGOFFSET_STATUS		0x01
#define LZ_I2CREGOFFSET_DEVICEADDRESS	0x02
#define LZ_I2CREGOFFSET_TRANSMITADDRESS	0x03
#define LZ_I2CREGOFFSET_DATAPORT	0x04
#define LZ_I2CREGOFFSET_CLOCKPRESCALE1	0x05
#define LZ_I2CREGOFFSET_CLOCKPRESCALE2	0x06
#define LZ_I2CREGOFFSET_EXTRACONTROL	0x07
#define LZ_I2CREGOFFSET_SEMAPHORE	0x08

#define CONTROL_SLAVENAK			0x00000001
#define CONTROL_MASTERSTOP			0x00000002
#define CONTROL_STOPINTERRUPT			0x00000004
#define CONTROL_ARBITRATIONLOSSINTERRUPT	0x00000008
#define CONTROL_NAKINTERRUPT			0x00000010
#define CONTROL_TRANSMITINTERRUPT		0x00000020
#define CONTROL_RECEIVEINTERRUPT		0x00000040
#define CONTROL_SLAVEWRITEINTERRUPT		0x00000080

#define CONTROL_SLAVENAK_SHIFT			(0)
#define CONTROL_MASTERSTOP_SHIFT		(1)
#define CONTROL_STOPINTERRUPT_SHIFT		(2)
#define CONTROL_ARBITRATIONLOSSINTERRUPT_SHIFT	(3)
#define CONTROL_NAKINTERRUPT_SHIFT		(4)
#define CONTROL_TRANSMITINTERRUPT_SHIFT		(5)
#define CONTROL_RECEIVEINTERRUPT_SHIFT		(6)
#define CONTROL_SLAVEWRITEINTERRUPT_SHIFT	(7)

#define CONTROL_SLAVENAK_RANGE			0x00000001
#define CONTROL_MASTERSTOP_RANGE		0x00000001
#define CONTROL_STOPINTERRUPT_RANGE		0x00000001
#define CONTROL_ARBITRATIONLOSSINTERRUPT_RANGE	0x00000001
#define CONTROL_NAKINTERRUPT_RANGE		0x00000001
#define CONTROL_TRANSMITINTERRUPT_RANGE		0x00000001
#define CONTROL_RECEIVEINTERRUPT_RANGE		0x00000001
#define CONTROL_SLAVEWRITEINTERRUPT_RANGE	0x00000001

#define STATUS_MASTERMODE			0x00000001
#define STATUS_BUSBUSY				0x00000002
#define STATUS_STOPINTERRUPT			0x00000004
#define STATUS_ARBITRATIONLOSSINTERRUPT		0x00000008
#define STATUS_NAKINTERRUPT			0x00000010
#define STATUS_TRANSMITINTERRUPT		0x00000020
#define STATUS_RECEIVEINTERRUPT			0x00000040
#define STATUS_SLAVEWRITEINTERRUPT		0x00000080

#define STATUS_INTERRUPT_MASK			0x000000FC

#define STATUS_MASTERMODE_SHIFT			(0)
#define STATUS_BUSBUSY_SHIFT			(1)
#define STATUS_STOPINTERRUPT_SHIFT		(2)
#define STATUS_ARBITRATIONLOSSINTERRUPT_SHIFT	(3)
#define STATUS_NAKINTERRUPT_SHIFT		(4)
#define STATUS_TRANSMITINTERRUPT_SHIFT		(5)
#define STATUS_RECEIVEINTERRUPT_SHIFT		(6)
#define STATUS_SLAVEWRITEINTERRUPT_SHIFT	(7)

#define STATUS_MASTERMODE_RANGE			0x00000001
#define STATUS_BUSBUSY_RANGE			0x00000001
#define STATUS_STOPINTERRUPT_RANGE		0x00000001
#define STATUS_ARBITRATIONLOSSINTERRUPT_RANGE	0x00000001
#define STATUS_NAKINTERRUPT_RANGE		0x00000001
#define STATUS_TRANSMITINTERRUPT_RANGE		0x00000001
#define STATUS_RECEIVEINTERRUPT_RANGE		0x00000001
#define STATUS_SLAVEWRITEINTERRUPT_RANGE	0x00000001

#define DEVICEADDRESS_DYNAMICSTARTSTOPTIMING	0x00000001
#define DEVICEADDRESS_ADDRESS			0x000000FE

#define DEVICEADDRESS_DYNAMICSTARTSTOPTIMING_SHIFT	(0)
#define DEVICEADDRESS_ADDRESS_SHIFT			(1)

#define DEVICEADDRESS_DYNAMICSTARTSTOPTIMING_RANGE	0x00000001
#define DEVICEADDRESS_ADDRESS_RANGE			0x0000007F


#define CLOCKPRESCALE2_PRESCALE2		0x0000007F
#define CLOCKPRESCALE2_FASTMODE			0x00000080

#define CLOCKPRESCALE2_PRESCALE2_SHIFT		(0)
#define CLOCKPRESCALE2_FASTMODE_SHIFT		(7)

#define CLOCKPRESCALE2_PRESCALE2_RANGE		0x0000007F
#define CLOCKPRESCALE2_FASTMODE_RANGE		0x00000001


#define EXTRACONTROL_RESETCLOCKDISABLE		0x00000001
#define EXTRACONTROL_DATAREADCONTROL		0x00000002
#define EXTRACONTROL_AUTOEXITDISABLE		0x00000004
#define EXTRACONTROL_MASTERCODEDISABLE		0x00000008
#define EXTRACONTROL_NODRIVEENABLE		0x00000010
#define EXTRACONTROL_NOFILTERENABLE		0x00000020
#define EXTRACONTROL_LATCHTIMEENABLE		0x00000040
#define EXTRACONTROL_RESET			0x00000080

#define EXTRACONTROL_RESETCLOCKDISABLE_SHIFT	(0)
#define EXTRACONTROL_DATAREADCONTROL_SHIFT	(1)
#define EXTRACONTROL_AUTOEXITDISABLE_SHIFT	(2)
#define EXTRACONTROL_MASTERCODEDISABLE_SHIFT	(3)
#define EXTRACONTROL_NODRIVEENABLE_SHIFT	(4)
#define EXTRACONTROL_NOFILTERENABLE_SHIFT	(5)
#define EXTRACONTROL_LATCHTIMEENABLE_SHIFT	(6)
#define EXTRACONTROL_RESET_SHIFT		(7)

#define EXTRACONTROL_RESETCLOCKDISABLE_RANGE	0x00000001
#define EXTRACONTROL_DATAREADCONTROL_RANGE	0x00000001
#define EXTRACONTROL_AUTOEXITDISABLE_RANGE	0x00000001
#define EXTRACONTROL_MASTERCODEDISABLE_RANGE	0x00000001
#define EXTRACONTROL_NODRIVEENABLE_RANGE	0x00000001
#define EXTRACONTROL_NOFILTERENABLE_RANGE	0x00000001
#define EXTRACONTROL_LATCHTIMEENABLE_RANGE	0x00000001
#define EXTRACONTROL_RESET_RANGE		0x00000001

#define SEMAPHORE_LOCK		0x00000001

#define SEMAPHORE_LOCK_SHIFT	(0)

#define SEMAPHORE_LOCK_RANGE	0x00000001

/*
 * ----
 * layer hardware driver
 */
/* lz_ layer zero */
static u8 lz_read_reg(void __iomem *reg_base, u32 reg)
{
	/* ioread8, ioread16, ioread32 */
	/* inb, inw, inl */
	return inb((unsigned long)reg_base + reg);
}

static void lz_write_reg(void __iomem *reg_base, u32 reg, u8 val)
{
	/* iowrite8, iowrite16, iowrite32 */
	/* outb, outw, outl */
	outb(val, (unsigned long)reg_base + reg);
}

struct i2cglobalconfigure {
	u32	dynamic_timing;
	u32	resetclock_disable;
	u32	dataread_control;
	u32	autoexit_disable;
	u32	mastercode_disable;
	u32	nodrive_enable;
	u32	nofilter_enable;
	u32	latchtime_enable;
};

struct i2cglobalstatus {
	u32	mastermode;
	u32	busbusy;
};

struct i2cinterrupt {
	u32	stop;
	u32	arbitrationloss;
	u32	nak;
	u32	transmit;
	u32	receive;
	u32	slavewrite;
};

struct i2cvolatileinfo {
	u32	dummy;
};

struct hwd_i2c {
	u32	portid;
	void __iomem	*reg_base;/* mapped mem or io address*/

	/* Driver Core Data */
	u32	wegetcontrol;
	u32	lasterror_arbitrationloss;
	u32	lasterror_nak;
};

static void hwdi2c_initialize0(
	IN struct hwd_i2c *this)
{
	/* int retval; return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	this->reg_base = NULL;
	this->wegetcontrol = FALSE;
}

static void hwdi2c_setmappedaddress(
	IN struct hwd_i2c *this,
	IN void __iomem *reg_base)
{
	this->reg_base = reg_base;
}

static void hwdi2c_opendevice(
	IN struct hwd_i2c *this,
	IN u32 portid)
{
	this->portid = portid;
}

static void hwdi2c_closedevice(
	IN struct hwd_i2c *this)
{
}

static int hwdi2c_inithw(
	IN struct hwd_i2c *this)
{
	return 0;
}

static int hwdi2c_isr(
	IN struct hwd_i2c *this,
	OUT u32 *pbinterrupt,
	OUT struct i2cinterrupt *pi2cinterrupt,
	OUT struct i2cvolatileinfo  *pi2cvolatileinfo)
{
	int retval; /* return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	u8 reg_interruptstatus;
	struct i2cinterrupt sinterruptstatus;

	u32 i;

	retval = 0;

	*pbinterrupt = FALSE;
	if (this->wegetcontrol == FALSE) {
		/* pr_err("    Interrupt wegetcontrol == FALSE\n"); */
		goto funcexit;
	}

	reg_interruptstatus = lz_read_reg(
		this->reg_base, LZ_I2CREGOFFSET_STATUS);
	/* to quickly handle interrupts. */
	/* reg_interruptstatus & STATUS_INTERRUPT_MASK */
	if ((reg_interruptstatus & STATUS_INTERRUPT_MASK) == 0) {
		goto funcexit;
	} else {
		*pbinterrupt = TRUE;
		for (i = 0; i < 4; i++)
			reg_interruptstatus = lz_read_reg(
				this->reg_base, LZ_I2CREGOFFSET_STATUS);
	}
	_getdatafield(reg_interruptstatus, STATUS_STOPINTERRUPT,
		sinterruptstatus.stop);
	_getdatafield(reg_interruptstatus, STATUS_ARBITRATIONLOSSINTERRUPT,
		sinterruptstatus.arbitrationloss);
	_getdatafield(reg_interruptstatus, STATUS_NAKINTERRUPT,
		sinterruptstatus.nak);
	_getdatafield(reg_interruptstatus, STATUS_TRANSMITINTERRUPT,
		sinterruptstatus.transmit);
	_getdatafield(reg_interruptstatus, STATUS_RECEIVEINTERRUPT,
		sinterruptstatus.receive);
	_getdatafield(reg_interruptstatus, STATUS_SLAVEWRITEINTERRUPT,
		sinterruptstatus.slavewrite);

	if (sinterruptstatus.arbitrationloss)
		this->lasterror_arbitrationloss = TRUE;

	if (sinterruptstatus.nak)
		this->lasterror_nak = TRUE;

	/* quickly clear interrupt that are handled. */
	lz_write_reg(
		this->reg_base, LZ_I2CREGOFFSET_STATUS, reg_interruptstatus);

	*pi2cinterrupt = sinterruptstatus;

	goto funcexit;
funcexit:

	return retval;
}

/*static int hwdi2c_dpcforisr(
 *	IN struct hwd_i2c *this,
 *	IN struct i2cinterrupt *pi2cinterrupt,
 *	IN struct i2cvolatileinfo  *pi2cvolatileinfo)
 *{
 *	return 0;
 *}
 */

static void hwdi2c_getinterruptstatus(
	IN struct hwd_i2c *this,
	IN OUT u8 *preg_interruptstatus,
	IN OUT struct i2cinterrupt *pi2cinterruptstatus)
{
	u8 reg_interruptstatus;

	reg_interruptstatus = lz_read_reg(
		this->reg_base, LZ_I2CREGOFFSET_STATUS);
	_getdatafield(reg_interruptstatus, STATUS_STOPINTERRUPT,
		pi2cinterruptstatus->stop);
	_getdatafield(reg_interruptstatus, STATUS_ARBITRATIONLOSSINTERRUPT,
		pi2cinterruptstatus->arbitrationloss);
	_getdatafield(reg_interruptstatus, STATUS_NAKINTERRUPT,
		pi2cinterruptstatus->nak);
	_getdatafield(reg_interruptstatus, STATUS_TRANSMITINTERRUPT,
		pi2cinterruptstatus->transmit);
	_getdatafield(reg_interruptstatus, STATUS_RECEIVEINTERRUPT,
		pi2cinterruptstatus->receive);
	_getdatafield(reg_interruptstatus, STATUS_SLAVEWRITEINTERRUPT,
		pi2cinterruptstatus->slavewrite);

	*preg_interruptstatus = reg_interruptstatus;
}

static void hwdi2c_clearinterruptstatus(
	IN struct hwd_i2c *this,
	IN u8 reg_interruptstatus)
{
	lz_write_reg(
		this->reg_base, LZ_I2CREGOFFSET_STATUS, reg_interruptstatus);
}

static void hwdi2c_enableinterrupt(
	IN struct hwd_i2c *this,
	IN struct i2cinterrupt *pi2cinterrupt)
{
	u8 reg_control;

	reg_control = lz_read_reg(
		this->reg_base, LZ_I2CREGOFFSET_CONTROL);
	if (pi2cinterrupt->stop)
		_setdatafield(reg_control, CONTROL_STOPINTERRUPT, 1);
	if (pi2cinterrupt->arbitrationloss == TRUE)
		_setdatafield(reg_control, CONTROL_ARBITRATIONLOSSINTERRUPT, 1);
	if (pi2cinterrupt->nak)
		_setdatafield(reg_control, CONTROL_NAKINTERRUPT, 1);
	if (pi2cinterrupt->transmit)
		_setdatafield(reg_control, CONTROL_TRANSMITINTERRUPT, 1);
	if (pi2cinterrupt->receive)
		_setdatafield(reg_control, CONTROL_RECEIVEINTERRUPT, 1);
	if (pi2cinterrupt->slavewrite)
		_setdatafield(reg_control, CONTROL_SLAVEWRITEINTERRUPT, 1);
	lz_write_reg(this->reg_base, LZ_I2CREGOFFSET_CONTROL, reg_control);
}

static void hwdi2c_disableinterrupt(
	IN struct hwd_i2c *this,
	IN struct i2cinterrupt *pi2cinterrupt)
{
	u8 reg_control;

	reg_control = lz_read_reg(this->reg_base, LZ_I2CREGOFFSET_CONTROL);
	if (pi2cinterrupt->stop)
		_setdatafield(reg_control, CONTROL_STOPINTERRUPT, 0);
	if (pi2cinterrupt->arbitrationloss)
		_setdatafield(reg_control, CONTROL_ARBITRATIONLOSSINTERRUPT, 0);
	if (pi2cinterrupt->nak)
		_setdatafield(reg_control, CONTROL_NAKINTERRUPT, 0);
	if (pi2cinterrupt->transmit)
		_setdatafield(reg_control, CONTROL_TRANSMITINTERRUPT, 0);
	if (pi2cinterrupt->receive)
		_setdatafield(reg_control, CONTROL_RECEIVEINTERRUPT, 0);
	if (pi2cinterrupt->slavewrite)
		_setdatafield(reg_control, CONTROL_SLAVEWRITEINTERRUPT, 0);
	lz_write_reg(this->reg_base, LZ_I2CREGOFFSET_CONTROL, reg_control);
}

static void hwdi2c_setglobalconfigure(
	IN struct hwd_i2c *this,
	IN struct i2cglobalconfigure *pi2cglobalconfigure)
{
	u8 reg_extracontrol;
	u8 reg_deviceaddress;

	reg_extracontrol = lz_read_reg(
		this->reg_base, LZ_I2CREGOFFSET_EXTRACONTROL);
	_setdatafield(reg_extracontrol, EXTRACONTROL_RESETCLOCKDISABLE,
		pi2cglobalconfigure->resetclock_disable);
	_setdatafield(reg_extracontrol, EXTRACONTROL_DATAREADCONTROL,
		pi2cglobalconfigure->dataread_control);
	_setdatafield(reg_extracontrol, EXTRACONTROL_AUTOEXITDISABLE,
		pi2cglobalconfigure->autoexit_disable);
	_setdatafield(reg_extracontrol, EXTRACONTROL_MASTERCODEDISABLE,
		pi2cglobalconfigure->mastercode_disable);
	_setdatafield(reg_extracontrol, EXTRACONTROL_NODRIVEENABLE,
		pi2cglobalconfigure->nodrive_enable);
	_setdatafield(reg_extracontrol, EXTRACONTROL_NOFILTERENABLE,
		pi2cglobalconfigure->nofilter_enable);
	_setdatafield(reg_extracontrol, EXTRACONTROL_LATCHTIMEENABLE,
		pi2cglobalconfigure->latchtime_enable);
	lz_write_reg(this->reg_base,
		LZ_I2CREGOFFSET_EXTRACONTROL, reg_extracontrol);

	reg_deviceaddress = lz_read_reg(
		this->reg_base, LZ_I2CREGOFFSET_DEVICEADDRESS);
	_setdatafield(reg_deviceaddress, DEVICEADDRESS_DYNAMICSTARTSTOPTIMING,
		pi2cglobalconfigure->dynamic_timing);
	lz_write_reg(this->reg_base,
		LZ_I2CREGOFFSET_DEVICEADDRESS, reg_deviceaddress);
}

static void hwdi2c_setclockprescale(
	IN struct hwd_i2c *this,
	IN u32 fastmode,
	IN u32 prescale1,
	IN u32 prescale2)
{
	u8 reg_prescale1;
	u8 reg_prescale2;

	reg_prescale1 = (u8)prescale1;
	lz_write_reg(
		this->reg_base, LZ_I2CREGOFFSET_CLOCKPRESCALE1, reg_prescale1);

	reg_prescale2 = 0;
	_setdatafield(reg_prescale2, CLOCKPRESCALE2_FASTMODE, fastmode);
	_setdatafield(reg_prescale2, CLOCKPRESCALE2_PRESCALE2, prescale2);
	lz_write_reg(
		this->reg_base, LZ_I2CREGOFFSET_CLOCKPRESCALE2, reg_prescale2);
}

static void hwdi2c_trylock(
	IN struct hwd_i2c *this,
	OUT u32 *pgetlock)
{
	u8 reg_semaphore;
	u32 lock;

	reg_semaphore = lz_read_reg(this->reg_base, LZ_I2CREGOFFSET_SEMAPHORE);
	_getdatafield(reg_semaphore, SEMAPHORE_LOCK, lock);
	if (lock) {
		*pgetlock = FALSE;
		this->wegetcontrol = FALSE;
	} else {
		*pgetlock = TRUE;
		this->wegetcontrol = TRUE;
	}
}

/*static void hwdi2c_unlock(
 *	IN struct hwd_i2c *this)
 *{
 *	u8 reg_semaphore;
 *
 *	this->wegetcontrol = FALSE;
 *
 *	reg_semaphore = 0;
 *	_setdatafield(reg_semaphore, SEMAPHORE_LOCK, 1);
 *	lz_write_reg(this->reg_base, LZ_I2CREGOFFSET_SEMAPHORE, reg_semaphore);
 *}
 */

static void hwdi2c_hardreset(
	IN struct hwd_i2c *this)
{
	struct timespec64 start;
	struct timespec64 end;
	u32 ms;

	u8 reg_extracontrol;
	u32 reset;

	this->wegetcontrol = FALSE;

	reg_extracontrol = lz_read_reg(
		this->reg_base, LZ_I2CREGOFFSET_EXTRACONTROL);
	_setdatafield(reg_extracontrol, EXTRACONTROL_RESET, 1);
	lz_write_reg(this->reg_base,
		LZ_I2CREGOFFSET_EXTRACONTROL, reg_extracontrol);

	/* wait for the action done. */
	ktime_get_real_ts64(&start);
	while (1) {
		reg_extracontrol = lz_read_reg(
			this->reg_base, LZ_I2CREGOFFSET_EXTRACONTROL);
		_getdatafield(reg_extracontrol, EXTRACONTROL_RESET, reset);
		if (reset == 0)
			break;

		ktime_get_real_ts64(&end);
		ms = (end.tv_sec - start.tv_sec)*1000;
		if ((end.tv_nsec - start.tv_nsec) != 0)
			ms += (end.tv_nsec - start.tv_nsec)/1000000;
		if (ms >= 1000) {
			pr_err("    %s timeout\n", __func__);
			break;
		}
	}
}

/*static void hwdi2c_start(
 *	IN struct hwd_i2c *this)
 *{
 *}
 *
 *static void hwdi2c_stop(
 *	IN struct hwd_i2c *this)
 *{
 *}
 */

static void hwdi2c_transmitaddress(
	IN struct hwd_i2c *this,
	IN u32 address)
{
	this->lasterror_arbitrationloss = 0;
	this->lasterror_nak = 0;
	lz_write_reg(this->reg_base, LZ_I2CREGOFFSET_TRANSMITADDRESS, address);
}

static void hwdi2c_writedataport(
	IN struct hwd_i2c *this,
	IN u32 data)
{
	this->lasterror_arbitrationloss = 0;
	this->lasterror_nak = 0;
	lz_write_reg(this->reg_base, LZ_I2CREGOFFSET_DATAPORT, data);
}

static void hwdi2c_readdataport(
	IN struct hwd_i2c *this,
	OUT u32 *pdata)
{
	this->lasterror_arbitrationloss = 0;
	this->lasterror_nak = 0;
	*pdata = lz_read_reg(this->reg_base, LZ_I2CREGOFFSET_DATAPORT);
}

static void hwdi2c_lasterror(
	IN struct hwd_i2c *this,
	OUT u32 *parbitrationerror,
	OUT u32 *pnak)
{
	*parbitrationerror = this->lasterror_arbitrationloss;
	*pnak = this->lasterror_nak;
}

static void hwdi2c_masterstop(
	IN struct hwd_i2c *this)
{
	u8 reg_control;

	reg_control = lz_read_reg(this->reg_base, LZ_I2CREGOFFSET_CONTROL);
	_setdatafield(reg_control, CONTROL_MASTERSTOP, 1);
	lz_write_reg(this->reg_base, LZ_I2CREGOFFSET_CONTROL, reg_control);
}

static void hwdi2c_checkmasterstop(
	IN struct hwd_i2c *this)
{
	struct timespec64 start;
	struct timespec64 end;
	u32 ms;

	u8 reg_control;
	u32 masterstop;

	/* wait for the action done. */
	ktime_get_real_ts64(&start);
	while (1) {
		reg_control = lz_read_reg(
			this->reg_base, LZ_I2CREGOFFSET_CONTROL);
		_getdatafield(reg_control, CONTROL_MASTERSTOP, masterstop);
		if (masterstop == 0)
			break;

		ktime_get_real_ts64(&end);
		ms = (end.tv_sec - start.tv_sec)*1000;
		if ((end.tv_nsec - start.tv_nsec) != 0)
			ms += (end.tv_nsec - start.tv_nsec)/1000000;
		if (ms >= 1000) {
			pr_err("    %s timeout\n", __func__);
			break;
		}
	}
}

struct spevent {
	struct completion	event;
	spinlock_t		lock;
	u32			trigger;
};

static void spevent_initialize(
	IN struct spevent *hspevent)
{
	init_completion(&hspevent->event);
	spin_lock_init(&hspevent->lock);
	hspevent->trigger = 0;
}

static void spevent_clear(
	IN struct spevent *hspevent)
{
	while (spin_trylock_bh(&hspevent->lock) == 0)
		schedule();
	hspevent->trigger = 0;
	spin_unlock_bh(&hspevent->lock);
}

static void spevent_set(
	IN struct spevent *hspevent)
{
	while (spin_trylock_bh(&hspevent->lock) == 0)
		schedule();
	hspevent->trigger = 1;
	spin_unlock_bh(&hspevent->lock);
	complete(&hspevent->event);
}

static void spevent_wait(
	IN struct spevent *hspevent,
	IN u32 timeoutms,
	OUT u32 *pbtimeout)
{
	u32 timeout;
	u32 trigger;

	*pbtimeout = FALSE;

	while (spin_trylock_bh(&hspevent->lock) == 0)
		schedule();
	trigger = hspevent->trigger;
	spin_unlock_bh(&hspevent->lock);

	if (trigger == 0) {
		timeout = wait_for_completion_interruptible_timeout(
			&hspevent->event, msecs_to_jiffies(timeoutms));
		if (timeout == 0)
			*pbtimeout = TRUE;
	}
}

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

	struct i2cinterrupt	losinterrupt[INTERRUPTQUEUE_MAXCOUNT];
	struct i2cvolatileinfo	losvolatileinfo[INTERRUPTQUEUE_MAXCOUNT];
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
	struct i2c_adapter	*i2cadapter;
	    /* container_of(adapter, struct module_port_device, i2cadapter); */
	char			*coredev_name;
		/* dev_name(ThisCoreDevice.dev) */

	/* device capability */

	/* monitor device status */

	/* device handles and resources */
	struct hwd_i2c		shwdi2c;
	struct hwd_i2c		*hwdi2c;
	struct i2cinterrupt	si2cinterrupt;

	u32			fastmode;
	u32			prescale1;
	u32			prescale2;
	u32			controladdress;
	u32			polling;

	struct i2cglobalconfigure	si2cglobalconfigure;

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

	struct spevent		spevent_null;
		/* use this to make sure schedule and wait */

	/* interrupt DPC events */
	struct spevent		spevent_transmitdone;
	struct spevent		spevent_receivedone;

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

static u32 rdc_i2c_1501_func(struct i2c_adapter *adap);
static int rdc_i2c_1501_access(
	struct i2c_adapter *adap,
	struct i2c_msg *msgs,
	int num_msgs);
static struct i2c_algorithm rdc_i2c_algorithm = {
	.master_xfer	= rdc_i2c_1501_access,
	.functionality	= rdc_i2c_1501_func,
};

static struct i2c_adapter thismodule_i2cadapter[] = {
	[0] = {
		.name	= "rdc_1501_i2c0",
		.owner	= THIS_MODULE,
		.class	= I2C_CLASS_HWMON | I2C_CLASS_SPD,
		.algo	= &rdc_i2c_algorithm,
	},

	[1] = {
		.name	= "rdc_1501_i2c1",
		.owner	= THIS_MODULE,
		.class	= I2C_CLASS_HWMON | I2C_CLASS_SPD,
		.algo	= &rdc_i2c_algorithm,
	},
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
	this_port_device->i2cadapter = NULL;
	this_port_device->hwdi2c = &this_port_device->shwdi2c;
	hwdi2c_initialize0(this_port_device->hwdi2c);

	memset(&this_port_device->si2cglobalconfigure, 0,
		sizeof(struct i2cglobalconfigure));

	tasklet_init(&this_port_device->tasklet_isrdpc, routine_isrdpc,
		(unsigned long)moduleportindex);
	spin_lock_init(&this_port_device->interruptlock_irq);
	spin_lock_init(&this_port_device->interruptlock_dpcqueue);
	memset(&this_port_device->sinterrupt_extension, 0,
		sizeof(struct interrupt_extension));

	spin_lock_init(&this_port_device->spinlock_dpc);

	spevent_initialize(&this_port_device->spevent_null);
	spevent_initialize(&this_port_device->spevent_transmitdone);
	spevent_initialize(&this_port_device->spevent_receivedone);

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
	unsigned int fastmode,
	unsigned int prescale1,
	unsigned int prescale2,
	unsigned int controladdress,
	unsigned int polling)
{
	int retval; /* return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	struct module_port_device *this_port_device;

	pr_info("%s Entry moduleportindex:%u\n", __func__, moduleportindex);
	retval = 0;
	this_port_device = &thismodule_port_devices[moduleportindex];

	this_port_device->fastmode = fastmode;
	this_port_device->prescale1 = prescale1;
	this_port_device->prescale2 = prescale2;
	this_port_device->controladdress = controladdress;
	this_port_device->polling = polling;

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

	struct module_port_device *this_port_device;
	struct i2c_adapter        *this_i2c_adapter;

	pr_info("%s Entry moduleportindex:%u\n", __func__, moduleportindex);
	retval = 0;

	pr_info("    dev_name %s\n", dev_name(dev)); /* ex: 0000:03:06.0 */

	/* pcidev = container_of(dev, struct pci_dev, device); */
	this_port_device = &thismodule_port_devices[moduleportindex];
	/* this_driver_data = dev_get_drvdata(dev); */
	/* this_specific = this_driver_data->this_specific; */

	/* allocate ThisCoreDevice */
	this_port_device->i2cadapter = &thismodule_i2cadapter[moduleportindex];

	/* init ThisCoreDevice */
	thismodule_i2cadapter[moduleportindex].dev.parent = dev;

	/* register ThisCoreDevice */
	result = i2c_add_adapter(&thismodule_i2cadapter[moduleportindex]);
	if (result != 0) {
		pr_err("    i2c_add_adapter failed %i\n", result);
		retval = -EFAULT;
		goto funcexit;
	}
	/* now, register is ok */
	this_port_device->coredev_name =
		(char *)dev_name(&this_port_device->i2cadapter->dev);
	pr_info("    register ThisCoreDevice ok, dev_name:%s\n",
		this_port_device->coredev_name);
	/* i2c_adapter != i2c_dev, i2c_dev have I2C_MAJOR and minor. */
	this_i2c_adapter = this_port_device->i2cadapter;
	if (MAJOR(this_i2c_adapter->dev.devt) != 0) {
		this_port_device->major = MAJOR(this_i2c_adapter->dev.devt);
		this_port_device->deviceid = MINOR(this_i2c_adapter->dev.devt);
		pr_info("    now, this port is registered with major:%u deviceid:%u\n",
			this_port_device->major, this_port_device->deviceid);
	} else {
		/*this_port_device->major = 0;
		 *this_port_device->deviceid = moduleportindex;
		 *pr_info("    this port have no major, so we let deviceid "
		 *"equal to moduleportindex:%u\n", this_port_device->deviceid);
		 */
		this_port_device->major = I2C_MAJOR;
		this_port_device->deviceid = this_i2c_adapter->nr;
		pr_info("    this i2c bus have major:%u, minor:%u\n",
			I2C_MAJOR, this_port_device->deviceid);
	}

	/* create device attributes for ThisCoreDevice */
	reterr = device_create_file(
		&this_i2c_adapter->dev, &dev_attr_drivername);
	if (reterr < 0) {
		pr_err("    device_create_file failed %i\n", reterr);
		retval = -EFAULT;
		goto funcexit;
	}
	/* reterr = device_create_file(dev_attr_others); */

	goto funcexit;
funcexit:

	if (retval != 0) {
		this_port_device->i2cadapter->dev.parent = NULL;
		this_port_device->i2cadapter = NULL;
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

	if (this_port_device->i2cadapter != NULL) {
		device_remove_file(&this_port_device->i2cadapter->dev,
			&dev_attr_drivername);

		/* unregister ThisCoreDevice */
		i2c_del_adapter(&thismodule_i2cadapter[moduleportindex]);

		this_port_device->i2cadapter->dev.parent = NULL;
		this_port_device->i2cadapter = NULL;
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
	hwdi2c_setmappedaddress(this_port_device->hwdi2c,
		this_port_device->reg_base);

	hwdi2c_opendevice(
		this_port_device->hwdi2c, this_port_device->portnumber);

	reterr = hwdi2c_inithw(this_port_device->hwdi2c);
	if (reterr < 0) {
		pr_err("    hwdi2c_inithw error:x%X\n", reterr);
		retval = -EFAULT;
		goto funcexit;
	} else {
		/* EvtDeviceD0Entry */
		/* hwd_start() */

		/* EvtDeviceD0EntryPostInterruptsEnabled */
		this_port_device->si2cinterrupt.stop = TRUE;
		this_port_device->si2cinterrupt.arbitrationloss = TRUE;
		this_port_device->si2cinterrupt.nak = TRUE;
		this_port_device->si2cinterrupt.transmit = TRUE;
		this_port_device->si2cinterrupt.receive = TRUE;
		this_port_device->si2cinterrupt.slavewrite = TRUE;
		/* hwdi2c_enableinterrupt() */
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
	/* hwdi2c_DisableInterrupt() */

	/* EvtDeviceD0Exit */
	/* hwdi2c_Stop() */
	/* hwdi2c_HardReset() */

	/* EvtDeviceReleaseHardware */
	hwdi2c_closedevice(this_port_device->hwdi2c);

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

	unsigned int                start;
	unsigned int                end;
	unsigned int                count;

	unsigned int                binterrupt;
	struct i2cinterrupt         sinterrupt = {0};
	struct i2cvolatileinfo      svolatileinfo = {0};

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
			continue;
		if (this_port_device->polling == 1)
			continue;

		/* now, handle isr*/
		memset(&sinterrupt, 0, sizeof(sinterrupt));
		memset(&svolatileinfo, 0, sizeof(svolatileinfo));
		binterrupt = FALSE;
		hwdi2c_isr(this_port_device->hwdi2c,
			&binterrupt, &sinterrupt, &svolatileinfo);
		if (binterrupt == FALSE)
			continue;

		/* pr_info("    binterrupt == TRUE\n"); */
		interruptserviced = TRUE;

		/* save InterruptVolatileInformation into DpcQueue */
		spin_lock(&this_port_device->interruptlock_dpcqueue);
		start = pinterrupt_ext->start;
		end = pinterrupt_ext->end;
		count = pinterrupt_ext->count;
		if (count == INTERRUPTQUEUE_MAXCOUNT) {
			/* the program should run to here. */
			pinterrupt_ext->overrun = TRUE;
		} else {
			pinterrupt_ext->overrun = FALSE;

			pinterrupt_ext->id += 1;
			pinterrupt_ext->losinterrupt[end] = sinterrupt;
			pinterrupt_ext->losvolatileinfo[end] = svolatileinfo;

			/* update end */
			pinterrupt_ext->end =
				(end + 1) % INTERRUPTQUEUE_MAXCOUNT;
			pinterrupt_ext->count += 1;
		}
		spin_unlock(&this_port_device->interruptlock_dpcqueue);

		if (interruptserviced == TRUE) {
			/* schedule EvtInterruptDpc */
			tasklet_schedule(&this_port_device->tasklet_isrdpc);
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
	/* int retval; return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	struct module_port_device   *this_port_device;
	struct interrupt_extension  *pinterrupt_ext;

	unsigned int                start;
	unsigned int                end;
	unsigned int                count;
	unsigned int                bempty;

	struct i2cinterrupt         sinterrupt = {0};
	struct i2cvolatileinfo      svolatileinfo = {0};

	this_port_device = &thismodule_port_devices[moduleportindex];
	pinterrupt_ext = &this_port_device->sinterrupt_extension;

	bempty = FALSE;
	while (bempty == FALSE) {
		/* get InterruptVolatileInformation from DpcQueue */
		spin_lock_irq(&this_port_device->interruptlock_dpcqueue);
		start = pinterrupt_ext->start;
		end = pinterrupt_ext->end;
		count = pinterrupt_ext->count;
		if (pinterrupt_ext->overrun == TRUE)
			pr_warn("    interrupt overrun\n");
		if (count > INTERRUPTQUEUE_MAXCOUNT)
			pr_warn("    interrupt max count\n");

		if (count == 0) {
			bempty = TRUE;
		} else {
			bempty = FALSE;

			/* get data from FIFO */
			sinterrupt = pinterrupt_ext->losinterrupt[start];
			svolatileinfo = pinterrupt_ext->losvolatileinfo[start];

			/* update start */
			pinterrupt_ext->start =
				(start + 1) % INTERRUPTQUEUE_MAXCOUNT;
			pinterrupt_ext->count -= 1;
		}
		spin_unlock_irq(&this_port_device->interruptlock_dpcqueue);

		if (bempty == TRUE)
			break;

		/* handle InterruptVolatileInformation */
		/* hwdi2c_DCPForISR() */

		if (sinterrupt.transmit == TRUE)
			spevent_set(&this_port_device->spevent_transmitdone);
		if (sinterrupt.receive == TRUE)
			spevent_set(&this_port_device->spevent_receivedone);

		if (pinterrupt_ext->start !=
			(start + 1) % INTERRUPTQUEUE_MAXCOUNT)
			pr_warn("    Interrupt_DPC sequence miss\n");
	}
}

static int rdc_i2c_controlwrite(
	struct module_port_device *this_port_device,
	struct i2c_msg *msgs);
static int rdc_i2c_controlread(
	struct module_port_device *this_port_device,
	struct i2c_msg *msgs);
static int rdc_i2c_masterread(
	struct module_port_device *this_port_device,
	struct i2c_msg *msgs,
	int blastmsg,
	int bhighspeed);
static int rdc_i2c_masterwrite(
	struct module_port_device *this_port_device,
	struct i2c_msg *msgs,
	int blastmsg,
	int bhighspeed);
static int rdc_i2c_masterread_polling(
	struct module_port_device *this_port_device,
	struct i2c_msg *msgs,
	int blastmsg,
	int bhighspeed);
static int rdc_i2c_masterwrite_polling(
	struct module_port_device *this_port_device,
	struct i2c_msg *msgs,
	int blastmsg,
	int bhighspeed);

static u32 rdc_i2c_1501_func(
	struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static int rdc_i2c_1501_access(
	struct i2c_adapter *adap,
	struct i2c_msg *msgs,
	int num_msgs)
{
	int retval; /* return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	unsigned int result; /* check if (result != 0) */

	unsigned int i;

	int                        moduleportindex;
	struct module_port_device  *this_port_device;

	unsigned int               getlock;

	unsigned int               btimeout;

	int                        blastmsg;
	int                        bhighspeed;

	retval = 0;
	this_port_device = NULL;
	getlock = FALSE;

	/* find this module port device & port index */
	for (i = 0; i < MAX_MODULE_PORTS; i++) {
		if (thismodule_port_devices[i].i2cadapter == adap) {
			moduleportindex = i;
			this_port_device = &thismodule_port_devices[i];
			break;
		}
	}
	if (this_port_device == NULL) {
		retval = -ENODEV;
		goto funcexit;
	}
	/* check if controladdress & num_msgs = 1 */
	if (msgs[0].addr == this_port_device->controladdress && num_msgs == 1) {
		if (msgs[0].flags & I2C_M_RD) {
			result = rdc_i2c_controlread(
				this_port_device, &msgs[0]);
		} else {
			result = rdc_i2c_controlwrite(
				this_port_device, &msgs[0]);
		}
		if (result != 0)
			retval = result;
		else
			retval += 1;
		goto funcexit;
	}

	/* try to lock hardware */
	i = 0;
	while (1) {
		getlock = FALSE;
		while (spin_trylock_irq(
			&this_port_device->interruptlock_irq) == 0)
			schedule();
		hwdi2c_trylock(this_port_device->hwdi2c, &getlock);
		spin_unlock_irq(&this_port_device->interruptlock_irq);
		if (getlock == TRUE)
			break;
		btimeout = FALSE;
		spevent_wait(&this_port_device->spevent_null, 1, &btimeout);
		i += 1;
		if (i == 1000) {
			pr_warn("    hwdi2c_trylock with getlock = FALSE\n");
			break;
		}
	}
	if (getlock == FALSE) {
		retval = -EAGAIN;
		goto funcexit;
	}

	/*now, we get lock */
	this_port_device->opencount = 1;

	hwdi2c_disableinterrupt(
		this_port_device->hwdi2c, &this_port_device->si2cinterrupt);

	hwdi2c_setclockprescale(
		this_port_device->hwdi2c, this_port_device->fastmode,
		this_port_device->prescale1, this_port_device->prescale2);

	hwdi2c_setglobalconfigure(this_port_device->hwdi2c,
		&this_port_device->si2cglobalconfigure);

	hwdi2c_enableinterrupt(
		this_port_device->hwdi2c, &this_port_device->si2cinterrupt);

	for (i = 0; i < num_msgs; i++) {
		blastmsg = FALSE;
		if (i == (num_msgs - 1))
			blastmsg = TRUE;
		bhighspeed = FALSE;
		if (i == 0 && (num_msgs != 1))
			if ((msgs[i].addr == 0x04) && (msgs[i].len == 0))
				bhighspeed = TRUE;
		if (msgs[i].flags & I2C_M_RD) {
			/* read */
			if (this_port_device->polling == 1)
				result = rdc_i2c_masterread_polling(
					this_port_device, &msgs[i],
					blastmsg, bhighspeed);
			else
				result = rdc_i2c_masterread(
					this_port_device, &msgs[i],
					blastmsg, bhighspeed);
		} else {
			/* write */
			if (this_port_device->polling == 1)
				result = rdc_i2c_masterwrite_polling(
					this_port_device, &msgs[i],
					blastmsg, bhighspeed);
			else
				result = rdc_i2c_masterwrite(
					this_port_device, &msgs[i],
					blastmsg, bhighspeed);
		}
		if (result != 0) {
			retval = result;
			goto funcexit;
		} else {
			retval += 1;
		}
	}


	goto funcexit;
funcexit:

	/* unlock hardware */
	if (getlock == TRUE) {
		this_port_device->opencount = 0;
		hwdi2c_checkmasterstop(this_port_device->hwdi2c);
		hwdi2c_hardreset(this_port_device->hwdi2c);
	}

	return retval;
}

static int rdc_i2c_controlread(
	struct module_port_device *this_port_device,
	struct i2c_msg *msgs)
{
	int retval; /* return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	pr_info("%s Entry\n", __func__);
	pr_info("    addr:x%02X flags:x%02X len:%u\n",
		msgs->addr, msgs->flags, msgs->len);
	pr_info("    empty function\n");

	retval = 0;

	goto funcexit;
funcexit:

	return retval;
}

static int rdc_i2c_controlwrite(
	struct module_port_device *this_port_device,
	struct i2c_msg *msgs)
{
	int retval; /* return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	pr_info("%s Entry\n", __func__);
	pr_info("    addr:x%02X flags:x%02X len:%u data: ",
		msgs->addr, msgs->flags, msgs->len);
	pr_info("    empty function\n");

	retval = 0;

	goto funcexit;
funcexit:

	return retval;
}

static int rdc_i2c_masterread(
	struct module_port_device *this_port_device,
	struct i2c_msg *msgs,
	int blastmsg,
	int bhighspeed)
{
	int retval; /* return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	unsigned int i;

	unsigned int    slaveaddress;
	struct spevent  *pspevent;
	unsigned int    btimeout;
	unsigned int    arbitrationloss;
	unsigned int    nak;
	unsigned int    databyte;
	/*pr_info("%s Entry\n", __func__);
	 *pr_info("    addr:x%02X flags:%02X len:%u\n",
	.*	msgs->addr, msgs->flags, msgs->len);
	 */
	retval = 0;

	/* send slaveaddress with read */
	/* clear the event before request */
	pspevent = &this_port_device->spevent_transmitdone;
	spevent_clear(pspevent);

	slaveaddress = (msgs->addr << 1) | 0x1; /* addr with rwbit(0x01) */
	hwdi2c_transmitaddress(this_port_device->hwdi2c, slaveaddress);

	/* wait for the event */
	btimeout = FALSE;
	spevent_wait(pspevent, 50, &btimeout);
	if (btimeout == TRUE) {
		DrvDbgShallNotRunToHere;
		hwdi2c_masterstop(this_port_device->hwdi2c);
		retval = -ETIMEDOUT;
		goto funcexit;
	}

	arbitrationloss = FALSE;
	nak = FALSE;
	hwdi2c_lasterror(this_port_device->hwdi2c, &arbitrationloss, &nak);
	if (arbitrationloss) {
		/* arbitrationloss occur in multi master system */
		/*pr_warn("    hwdi2c_transmitaddress with "
		 *	"arbitrationloss\n");
		 */
		hwdi2c_masterstop(this_port_device->hwdi2c);
		retval = -EAGAIN;
		goto funcexit;
	}
	if (nak) {
		if (bhighspeed) {
			/*pr_info("    hwdi2c_transmitaddress with "
			 *	"highspeed NAK\n");
			 */
		} else {
			pr_warn("    hwdi2c_transmitaddress with nak\n");
			hwdi2c_masterstop(this_port_device->hwdi2c);
			retval = -EIO;
			goto funcexit;
		}
	}

	if (msgs->len == 0) {
		if (blastmsg == TRUE)
			hwdi2c_masterstop(this_port_device->hwdi2c);
		goto funcexit;
	}

	/* read Bytes with STOP */
	for (i = 0; i < msgs->len; i++) {
		if (i == (msgs->len - 1) && blastmsg == TRUE)
			hwdi2c_masterstop(this_port_device->hwdi2c);

		/* clear the event before request */
		pspevent = &this_port_device->spevent_receivedone;
		spevent_clear(pspevent);

		hwdi2c_readdataport(this_port_device->hwdi2c, &databyte);
		if (i > 0)/* the first data is dummy data. */
			msgs->buf[i-1] = (u8)databyte;

		/* wait for the event */
		btimeout = FALSE;
		spevent_wait(pspevent, 50, &btimeout);
		if (btimeout == TRUE) {
			DrvDbgShallNotRunToHere;
			retval = -ETIMEDOUT;
			goto funcexit;
		}
	}
	/* the least data */
	hwdi2c_readdataport(this_port_device->hwdi2c, &databyte);
	msgs->buf[i-1] = (u8)databyte;

	/*pr_info(0, "    output data:");
	 *for (i = 0; i < msgs->len; i++)
	 *	pr_info(0, "%02X ", msgs->buf[i]);
	 *pr_info(0, "\n");
	 */

	goto funcexit;
funcexit:

	return retval;
}

static int rdc_i2c_masterwrite(
	struct module_port_device *this_port_device,
	struct i2c_msg *msgs,
	int blastmsg,
	int bhighspeed)
{
	int retval; /* return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	unsigned int i;

	unsigned int    slaveaddress;
	struct spevent  *pspevent;
	unsigned int    btimeout;
	unsigned int    arbitrationloss;
	unsigned int    nak;
	unsigned int    databyte;
	/*pr_info("%s Entry\n", __func__);
	 *pr_info("    addr:x%02X flags:x%02X len:%u data: ",
	 *	msgs->addr, msgs->flags, msgs->len);
	 *for (i = 0; i < msgs->len; i++)
	 *	pr_info(0, "%02X ", msgs->buf[i]);
	 *pr_info(0, "\n");
	 */

	retval = 0;

	/* send slaveaddress with write */
	/* clear the event before request */
	pspevent = &this_port_device->spevent_transmitdone;
	spevent_clear(pspevent);

	slaveaddress = msgs->addr << 1; /* addr with rwbit(0x00) */
	hwdi2c_transmitaddress(this_port_device->hwdi2c, slaveaddress);

	/* wait for the event */
	btimeout = FALSE;
	spevent_wait(pspevent, 50, &btimeout);
	if (btimeout == TRUE) {
		DrvDbgShallNotRunToHere;
		hwdi2c_masterstop(this_port_device->hwdi2c);
		retval = -ETIMEDOUT;
		goto funcexit;
	}

	arbitrationloss = FALSE;
	nak = FALSE;
	hwdi2c_lasterror(this_port_device->hwdi2c, &arbitrationloss, &nak);
	if (arbitrationloss) {
		/* arbitrationloss occur in multi master system */
		/*pr_warn("    hwdi2c_transmitaddress with "
		 *	"arbitrationloss\n");
		 */
		hwdi2c_masterstop(this_port_device->hwdi2c);
		retval = -EAGAIN;
		goto funcexit;
	}
	if (nak) {
		if (bhighspeed) {
			/*pr_info("    hwdi2c_transmitaddress with "
			 *	"highspeed NAK\n")
			 */
		} else {
			pr_warn("    hwdi2c_transmitaddress with nak\n");
			hwdi2c_masterstop(this_port_device->hwdi2c);
			retval = -EIO;
			goto funcexit;
		}
	}

	if (msgs->len == 0) {
		if (blastmsg == TRUE)
			hwdi2c_masterstop(this_port_device->hwdi2c);
		goto funcexit;
	}

	/* send bytes with STOP */
	for (i = 0; i < msgs->len; i++) {
		/* clear the event before request */
		pspevent = &this_port_device->spevent_transmitdone;
		spevent_clear(pspevent);

		databyte = msgs->buf[i];
		hwdi2c_writedataport(this_port_device->hwdi2c, databyte);

		/* wait for the event */
		btimeout = FALSE;
		spevent_wait(pspevent, 50, &btimeout);
		if (btimeout == TRUE) {
			DrvDbgShallNotRunToHere;
			hwdi2c_masterstop(this_port_device->hwdi2c);
			retval = -ETIMEDOUT;
			goto funcexit;
		}

		arbitrationloss = FALSE;
		nak = FALSE;
		hwdi2c_lasterror(this_port_device->hwdi2c,
			&arbitrationloss, &nak);
		if (nak) {
			pr_info("    hwdi2c_writedataport with nak\n");
			hwdi2c_masterstop(this_port_device->hwdi2c);
			retval = -EIO;
			goto funcexit;
		}
	}
	if (blastmsg == TRUE)
		hwdi2c_masterstop(this_port_device->hwdi2c);

	goto funcexit;
funcexit:

	return retval;
}

static int rdc_i2c_masterread_polling(
	struct module_port_device *this_port_device,
	struct i2c_msg *msgs,
	int blastmsg,
	int bhighspeed)
{
	int retval; /* return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	unsigned int i;

	unsigned int    slaveaddress;
	/*struct spevent  *pspevent;*/
	unsigned int    btimeout;
	unsigned int    arbitrationloss;
	unsigned int    nak;
	unsigned int    databyte;

	u8 reg_interruptstatus;
	struct i2cinterrupt sinterruptstatus;
	unsigned int    transmitdone;
	unsigned int    receivedone;
	/*pr_info("%s Entry\n", __func__);
	 *pr_info("    addr:x%02X flags:%02X len:%u\n",
	 *	msgs->addr, msgs->flags, msgs->len);
	 */
	retval = 0;

	/* send slaveaddress with read */
	slaveaddress = (msgs->addr << 1) | 0x1; /* addr with rwbit(0x01) */
	hwdi2c_transmitaddress(this_port_device->hwdi2c, slaveaddress);

	/* wait for the event */
	transmitdone = FALSE;
	arbitrationloss = FALSE;
	nak = FALSE;
	btimeout = 0;
	while (1) {
		hwdi2c_getinterruptstatus(this_port_device->hwdi2c,
			&reg_interruptstatus, &sinterruptstatus);

		/* to quickly handle interrupts. */
		if (reg_interruptstatus & STATUS_INTERRUPT_MASK) {
			if (sinterruptstatus.transmit == TRUE)
				transmitdone += 1;
			if (sinterruptstatus.arbitrationloss == TRUE)
				arbitrationloss = TRUE;
			if (sinterruptstatus.nak == TRUE)
				nak = TRUE;
			/* quickly clear interrupt that are handled. */
			if (transmitdone == 4) {
				hwdi2c_clearinterruptstatus(
					this_port_device->hwdi2c,
					reg_interruptstatus);
				break;
			}
		}
		btimeout += 1;
		if (btimeout == 100) {
			pr_err("    hwdi2c_transmitaddress with btimeout\n");
			hwdi2c_masterstop(this_port_device->hwdi2c);
			retval = -ETIMEDOUT;
			goto funcexit;
		}
	}

	if (arbitrationloss) {
		/* arbitrationloss occur in multi master system */
		/*pr_warn("    hwdi2c_transmitaddress with "
		 *	"arbitrationloss\n");
		 */
		hwdi2c_masterstop(this_port_device->hwdi2c);
		retval = -EAGAIN;
		goto funcexit;
	}
	if (nak) {
		if (bhighspeed) {
			/*pr_info("    hwdi2c_transmitaddress with "
			 *	"highspeed NAK\n");
			 */
		} else {
			/*pr_warn("    hwdi2c_transmitaddress with nak\n");*/
			hwdi2c_masterstop(this_port_device->hwdi2c);
			retval = -EIO;
			goto funcexit;
		}
	}

	if (msgs->len == 0) {
		if (blastmsg == TRUE)
			hwdi2c_masterstop(this_port_device->hwdi2c);
		goto funcexit;
	}

	/* read Bytes with STOP */
	for (i = 0; i < msgs->len; i++) {
		if (i == (msgs->len - 1) && blastmsg == TRUE)
			hwdi2c_masterstop(this_port_device->hwdi2c);

		hwdi2c_readdataport(this_port_device->hwdi2c, &databyte);
		if (i > 0) /* the first data is dummy data. */
			msgs->buf[i-1] = (u8)databyte;

		/* wait for the event */
		receivedone = FALSE;
		btimeout = 0;
		while (1) {
			hwdi2c_getinterruptstatus(this_port_device->hwdi2c,
				&reg_interruptstatus, &sinterruptstatus);

			/* to quickly handle interrupts. */
			if (reg_interruptstatus & STATUS_INTERRUPT_MASK) {
				if (sinterruptstatus.receive == TRUE)
					receivedone = TRUE;
				/* quickly clear interrupt that are handled. */
				if (receivedone == TRUE) {
					hwdi2c_clearinterruptstatus(
						this_port_device->hwdi2c,
						reg_interruptstatus);
					break;
				}
			}
			btimeout += 1;
			if (btimeout == 100) {
				pr_err("    hwdi2c_readdataport with btimeout\n");
				hwdi2c_masterstop(this_port_device->hwdi2c);
				retval = -ETIMEDOUT;
				goto funcexit;
			}
		}
	}
	/* the least data */
	hwdi2c_readdataport(this_port_device->hwdi2c, &databyte);
	msgs->buf[i-1] = (u8)databyte;

	/*pr_info(0, "    output data:");
	 *for (i = 0; i < msgs->len; i++)
	 *	pr_info("%02X ", msgs->buf[i]);
	 *pr_info("\n");
	 */

	goto funcexit;
funcexit:

	return retval;
}

static int rdc_i2c_masterwrite_polling(
	struct module_port_device *this_port_device,
	struct i2c_msg *msgs,
	int blastmsg,
	int bhighspeed)
{
	int retval; /* return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	unsigned int i;

	unsigned int    slaveaddress;
	/*struct spevent  *pspevent;*/
	unsigned int    btimeout;
	unsigned int    arbitrationloss;
	unsigned int    nak;
	unsigned int    databyte;

	u8 reg_interruptstatus;
	struct i2cinterrupt sinterruptstatus;
	unsigned int    transmitdone;
	/*unsigned int    receivedone;*/
	/*pr_info("%s Entry\n", __func__);
	 *pr_info("    addr:x%02X flags:x%02X len:%u data: ",
	 *	msgs->addr, msgs->flags, msgs->len);
	 *for (i = 0; i < msgs->len; i++)
	 *	pr_info("%02X ", msgs->buf[i]);
	 *pr_info("\n");
	 */
	retval = 0;

	/* send slaveaddress with write */
	slaveaddress = msgs->addr << 1; /* addr with rwbit(0x00) */
	hwdi2c_transmitaddress(this_port_device->hwdi2c, slaveaddress);

	/* wait for the event */
	transmitdone = FALSE;
	arbitrationloss = FALSE;
	nak = FALSE;
	btimeout = 0;
	while (1) {
		hwdi2c_getinterruptstatus(this_port_device->hwdi2c,
			&reg_interruptstatus, &sinterruptstatus);

		/* to quickly handle interrupts. */
		if (reg_interruptstatus & STATUS_INTERRUPT_MASK) {
			if (sinterruptstatus.transmit == TRUE)
				transmitdone += 1;
			if (sinterruptstatus.arbitrationloss == TRUE)
				arbitrationloss = TRUE;
			if (sinterruptstatus.nak == TRUE)
				nak = TRUE;
			/* quickly clear interrupt that are handled. */
			if (transmitdone == 4) {
				hwdi2c_clearinterruptstatus(
					this_port_device->hwdi2c,
					reg_interruptstatus);
				break;
			}
		}
		btimeout += 1;
		if (btimeout == 100) {
			pr_err("    hwdi2c_transmitaddress with btimeout\n");
			hwdi2c_masterstop(this_port_device->hwdi2c);
			retval = -ETIMEDOUT;
			goto funcexit;
		}
	}

	if (arbitrationloss) {
		/* arbitrationloss occur in multi master system */
		/*pr_warn("    hwdi2c_transmitaddress with "
		 *	"arbitrationloss\n");
		 */
		hwdi2c_masterstop(this_port_device->hwdi2c);
		retval = -EAGAIN;
		goto funcexit;
	}
	if (nak) {
		if (bhighspeed) {
			/*pr_info("    hwdi2c_transmitaddress with "
			 *	"highspeed NAK\n")
			 */
		} else {
			/*pr_warn("    hwdi2c_transmitaddress with nak\n");*/
			hwdi2c_masterstop(this_port_device->hwdi2c);
			retval = -EIO;
			goto funcexit;
		}
	}

	if (msgs->len == 0) {
		if (blastmsg == TRUE)
			hwdi2c_masterstop(this_port_device->hwdi2c);
		goto funcexit;
	}

	/* send bytes with STOP */
	for (i = 0; i < msgs->len; i++) {
		databyte = msgs->buf[i];
		hwdi2c_writedataport(this_port_device->hwdi2c, databyte);

		/* wait for the event */
		transmitdone = FALSE;
		arbitrationloss = FALSE;
		nak = FALSE;
		btimeout = 0;
		while (1) {
			hwdi2c_getinterruptstatus(this_port_device->hwdi2c,
				&reg_interruptstatus, &sinterruptstatus);

			/* to quickly handle interrupts. */
			if (reg_interruptstatus & STATUS_INTERRUPT_MASK) {
				if (sinterruptstatus.transmit == TRUE)
					transmitdone += 1;
				if (sinterruptstatus.arbitrationloss == TRUE)
					arbitrationloss = TRUE;
				if (sinterruptstatus.nak == TRUE)
					nak = TRUE;
				/* quickly clear interrupt that are handled. */
				if (transmitdone == 4) {
					hwdi2c_clearinterruptstatus(
						this_port_device->hwdi2c,
						reg_interruptstatus);
					break;
				}
			}
			btimeout += 1;
			if (btimeout == 100) {
				pr_err("    hwdi2c_writedataport with btimeout\n");
				hwdi2c_masterstop(this_port_device->hwdi2c);
				retval = -ETIMEDOUT;
				goto funcexit;
			}
		}
		if (nak) {
			pr_info("    hwdi2c_writedataport with nak\n");
			hwdi2c_masterstop(this_port_device->hwdi2c);
			retval = -EIO;
			goto funcexit;
		}
	}
	if (blastmsg == TRUE)
		hwdi2c_masterstop(this_port_device->hwdi2c);

	goto funcexit;
funcexit:

	return retval;
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
MODULE_DESCRIPTION("RDC Platform I2C Controller Driver");
MODULE_VERSION(DRIVER_VERSION);

/*
 * ----
 * define module parameters: mp_
 * if perm is 0, no sysfs entry.
 * if perm is S_IRUGO 0444, there is a sysfs entry, but cant altered.
 * if perm is S_IRUGO | S_IWUSR, there is a sysfs entry and can altered by root.
 */
static int mp_fastmode; /*static variable always zero*/
static int mp_prescale1 = 15;
static int mp_prescale2 = 32;
static int mp_controladdress;
static int mp_polling;

MODULE_PARM_DESC(mp_fastmode, " FastMode: 0 or 1 (default:0)");
module_param(mp_fastmode, int, 0444);
MODULE_PARM_DESC(mp_prescale1, " PreScale1: 15, 15 (default:15)");
module_param(mp_prescale1, int, 0444);
MODULE_PARM_DESC(mp_prescale2, " PreScale2: 32, 8 (default:32)");
module_param(mp_prescale2, int, 0444);
MODULE_PARM_DESC(mp_controladdress, " ControlAddress: (default:0) ");
module_param(mp_controladdress, int, 0444);
MODULE_PARM_DESC(mp_polling, " Polling: (default:0) ");
module_param(mp_polling, int, 0444);

/*
 * ----
 * define multiple pnp devices for this module supports
 */
#define MAX_SPECIFIC	1 /* this module support */
enum platform_specific_num_t {
	platform_17f3_1501 = 0, /* fixed on board */
};

static int platform_default_init(
	struct platform_driver_data *this_driver_data);
static int platform_default_setup(
	struct platform_driver_data *this_driver_data);
static void platform_default_exit(
	struct platform_driver_data *this_driver_data);

static struct platform_specific thismodule_platform_specifics[] = {
	[platform_17f3_1501] = {
		.vendor		= 0x17f3,
		.device		= 0x1501,
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
		.reg_length	= 9,
	},
};

/*
 * ----
 * define pnp device table for this module supports
 * PNP_ID_LEN = 8
 */
static const struct pnp_device_id pnp_device_id_table[] = {
	{ .id = "PNP1501", .driver_data = platform_17f3_1501, },
	{ .id = "AHC0510", .driver_data = platform_17f3_1501, },
	{ } /* terminate list */
};
MODULE_DEVICE_TABLE(pnp, pnp_device_id_table);

static const struct platform_device_id platform_device_id_table[] = {
	/* we take driver_data as a specific number(ID). */
	{ .name = "PNP1501", .driver_data = platform_17f3_1501, },
	{ .name = "AHC0510", .driver_data = platform_17f3_1501, },
	{ } /* terminate list */
};
MODULE_DEVICE_TABLE(platform, platform_device_id_table);

static const struct of_device_id of_device_id_table[] = {
	{ .compatible = "rdc,ahc-i2c", .data = platform_17f3_1501, },
	{},
};
MODULE_DEVICE_TABLE(of, of_device_id_table);

#ifdef CONFIG_ACPI
static const struct acpi_device_id acpi_device_id_table[] = {
	{ "PNP1501", platform_17f3_1501, },
	{ "AHC0510", platform_17f3_1501, },
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
			mp_fastmode, mp_prescale1, mp_prescale2,
			mp_controladdress, mp_polling);

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

