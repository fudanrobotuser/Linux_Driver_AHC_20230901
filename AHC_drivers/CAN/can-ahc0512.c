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
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/can.h>
#include <linux/can/dev.h>
#include <linux/version.h>

/* our device header */
/*
 * return -ERRNO -EFAULT -ENOMEM -EINVAL
 * device -EBUSY, -ENODEV, -ENXIO
 * parameter -EOPNOTSUPP
 * pr_info pr_warn pr_err
 */
#define DRIVER_NAME		"RDC CAN"
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

#define LZ_CANREGOFFSET_GLOBALCONTROL		0
#define LZ_CANREGOFFSET_CLOCKPRESCALER		4
#define LZ_CANREGOFFSET_BUSTIMING		8
#define LZ_CANREGOFFSET_INTERRUPTENABLE		12
#define LZ_CANREGOFFSET_INTERRUPTSTATUS		16
#define LZ_CANREGOFFSET_GLOBALSTATUS		20
#define LZ_CANREGOFFSET_REQUEST			24
#define LZ_CANREGOFFSET_TXBUFFER0STATUS		28
#define LZ_CANREGOFFSET_TXBUFFER1STATUS		32
#define LZ_CANREGOFFSET_TXBUFFER2STATUS		36
#define LZ_CANREGOFFSET_RECEIVESTATUS		40
#define LZ_CANREGOFFSET_ERRORWARNINGLIMIT	44
#define LZ_CANREGOFFSET_ERRORCOUNTER		48
#define LZ_CANREGOFFSET_IDINDEX			52
#define LZ_CANREGOFFSET_IDFILTER		56
#define LZ_CANREGOFFSET_IDMASK			60
#define LZ_CANREGOFFSET_TFI0			64
#define LZ_CANREGOFFSET_TID0			68
#define LZ_CANREGOFFSET_TDL0			72
#define LZ_CANREGOFFSET_TDH0			76
#define LZ_CANREGOFFSET_TFI1			80
#define LZ_CANREGOFFSET_TID1			84
#define LZ_CANREGOFFSET_TDL1			88
#define LZ_CANREGOFFSET_TDH1			92
#define LZ_CANREGOFFSET_TFI2			96
#define LZ_CANREGOFFSET_TID2			100
#define LZ_CANREGOFFSET_TDL2			104
#define LZ_CANREGOFFSET_TDH2			108
#define LZ_CANREGOFFSET_RFI			112
#define LZ_CANREGOFFSET_RID			116
#define LZ_CANREGOFFSET_RDL			120
#define LZ_CANREGOFFSET_RDH			124

#define GLOBALCONTROL_RESET				0x00000001
#define GLOBALCONTROL_ACTIVE				0x00000002
#define GLOBALCONTROL_SILENT				0x00000004
#define GLOBALCONTROL_LOOPBACK				0x00000008
#define GLOBALCONTROL_TRANSMITWITHNOACK			0x00000010
#define GLOBALCONTROL_SELFRECEPTION			0x00000020
#define GLOBALCONTROL_TRANSMITPRIORITY			0x000000C0
#define GLOBALCONTROL_ARBITRATIONLOSTRETRY		0x00000100
#define GLOBALCONTROL_BUSERRORRETRY			0x00000200
#define GLOBALCONTROL_SPECIAL1				0x00000400
#define GLOBALCONTROL_IDENTIFIERRESET			0x00010000
#define GLOBALCONTROL_IDENTIFIERENABLE			0x00020000
#define GLOBALCONTROL_IDENTIFIERBYPASS			0x00040000
#define GLOBALCONTROL_POWERSAVING			0x01000000
#define GLOBALCONTROL_RXBUSERROR			0x02000000

#define GLOBALCONTROL_RESET_SHIFT			(0)
#define GLOBALCONTROL_ACTIVE_SHIFT			(1)
#define GLOBALCONTROL_SILENT_SHIFT			(2)
#define GLOBALCONTROL_LOOPBACK_SHIFT			(3)
#define GLOBALCONTROL_TRANSMITWITHNOACK_SHIFT		(4)
#define GLOBALCONTROL_SELFRECEPTION_SHIFT		(5)
#define GLOBALCONTROL_TRANSMITPRIORITY_SHIFT		(6)
#define GLOBALCONTROL_ARBITRATIONLOSTRETRY_SHIFT	(8)
#define GLOBALCONTROL_BUSERRORRETRY_SHIFT		(9)
#define GLOBALCONTROL_SPECIAL1_SHIFT			(10)
#define GLOBALCONTROL_IDENTIFIERRESET_SHIFT		(16)
#define GLOBALCONTROL_IDENTIFIERENABLE_SHIFT		(17)
#define GLOBALCONTROL_IDENTIFIERBYPASS_SHIFT		(18)
#define GLOBALCONTROL_POWERSAVING_SHIFT			(24)
#define GLOBALCONTROL_RXBUSERROR_SHIFT			(25)

#define GLOBALCONTROL_RESET_RANGE			0x00000001
#define GLOBALCONTROL_ACTIVE_RANGE			0x00000001
#define GLOBALCONTROL_SILENT_RANGE			0x00000001
#define GLOBALCONTROL_LOOPBACK_RANGE			0x00000001
#define GLOBALCONTROL_TRANSMITWITHNOACK_RANGE		0x00000001
#define GLOBALCONTROL_SELFRECEPTION_RANGE		0x00000001
#define GLOBALCONTROL_TRANSMITPRIORITY_RANGE		0x00000003
#define GLOBALCONTROL_ARBITRATIONLOSTRETRY_RANGE	0x00000001
#define GLOBALCONTROL_BUSERRORRETRY_RANGE		0x00000001
#define GLOBALCONTROL_SPECIAL1_RANGE			0x00000001
#define GLOBALCONTROL_IDENTIFIERRESET_RANGE		0x00000001
#define GLOBALCONTROL_IDENTIFIERENABLE_RANGE		0x00000001
#define GLOBALCONTROL_IDENTIFIERBYPASS_RANGE		0x00000001
#define GLOBALCONTROL_POWERSAVING_RANGE			0x00000001
#define GLOBALCONTROL_RXBUSERROR_RANGE			0x00000001

#define CLOCKPRESCALER_CKDIV			0x0000FFFF
#define CLOCKPRESCALER_EXTERNALINPUT		0x80000000

#define CLOCKPRESCALER_CKDIV_SHIFT		(0)
#define CLOCKPRESCALER_EXTERNALINPUT_SHIFT	(31)

#define CLOCKPRESCALER_CKDIV_RANGE		0x0000FFFF
#define CLOCKPRESCALER_EXTERNALINPUT_RANGE	0x00000001

#define BUSTIMING_PROPAGATIONSEG		0x00000007
#define BUSTIMING_PHASESEG1			0x00000070
#define BUSTIMING_PHASESEG2			0x00000700
#define BUSTIMING_SJW				0x00003000
#define BUSTIMING_SAMPLING			0x00008000

#define BUSTIMING_PROPAGATIONSEG_SHIFT		(0)
#define BUSTIMING_PHASESEG1_SHIFT		(4)
#define BUSTIMING_PHASESEG2_SHIFT		(8)
#define BUSTIMING_SJW_SHIFT			(12)
#define BUSTIMING_SAMPLING_SHIFT		(15)

#define BUSTIMING_PROPAGATIONSEG_RANGE		0x00000007
#define BUSTIMING_PHASESEG1_RANGE		0x00000007
#define BUSTIMING_PHASESEG2_RANGE		0x00000007
#define BUSTIMING_SJW_RANGE			0x00000003
#define BUSTIMING_SAMPLING_RANGE		0x00000001

#define INTERRUPTENABLE_RECEIVE				0x00000001
#define INTERRUPTENABLE_TXBUFFER0TRANSMIT		0x00000002
#define INTERRUPTENABLE_TXBUFFER1TRANSMIT		0x00000004
#define INTERRUPTENABLE_TXBUFFER2TRANSMIT		0x00000008
#define INTERRUPTENABLE_ERRORWARNING			0x00000010
#define INTERRUPTENABLE_ERRORPASSIVE			0x00000020
#define INTERRUPTENABLE_BUSOFF				0x00000040
#define INTERRUPTENABLE_ARBITRATIONLOST			0x00000080
#define INTERRUPTENABLE_RXBUSERROR			0x00000100
#define INTERRUPTENABLE_RXFIFOOVERRUN			0x00000200
#define INTERRUPTENABLE_POWERSAVINGEXIT			0x00000400

#define INTERRUPTENABLE_RECEIVE_SHIFT			(0)
#define INTERRUPTENABLE_TXBUFFER0TRANSMIT_SHIFT		(1)
#define INTERRUPTENABLE_TXBUFFER1TRANSMIT_SHIFT		(2)
#define INTERRUPTENABLE_TXBUFFER2TRANSMIT_SHIFT		(3)
#define INTERRUPTENABLE_ERRORWARNING_SHIFT		(4)
#define INTERRUPTENABLE_ERRORPASSIVE_SHIFT		(5)
#define INTERRUPTENABLE_BUSOFF_SHIFT			(6)
#define INTERRUPTENABLE_ARBITRATIONLOST_SHIFT		(7)
#define INTERRUPTENABLE_RXBUSERROR_SHIFT		(8)
#define INTERRUPTENABLE_RXFIFOOVERRUN_SHIFT		(9)
#define INTERRUPTENABLE_POWERSAVINGEXIT_SHIFT		(10)

#define INTERRUPTENABLE_RECEIVE_RANGE			0x00000001
#define INTERRUPTENABLE_TXBUFFER0TRANSMIT_RANGE		0x00000001
#define INTERRUPTENABLE_TXBUFFER1TRANSMIT_RANGE		0x00000001
#define INTERRUPTENABLE_TXBUFFER2TRANSMIT_RANGE		0x00000001
#define INTERRUPTENABLE_ERRORWARNING_RANGE		0x00000001
#define INTERRUPTENABLE_ERRORPASSIVE_RANGE		0x00000001
#define INTERRUPTENABLE_BUSOFF_RANGE			0x00000001
#define INTERRUPTENABLE_ARBITRATIONLOST_RANGE		0x00000001
#define INTERRUPTENABLE_RXBUSERROR_RANGE		0x00000001
#define INTERRUPTENABLE_RXFIFOOVERRUN_RANGE		0x00000001
#define INTERRUPTENABLE_POWERSAVINGEXIT_RANGE		0x00000001

#define STATUS_INTERRUPT_MASK			0x7FF

#define INTERRUPTSTATUS_RECEIVE				0x00000001
#define INTERRUPTSTATUS_TXBUFFER0TRANSMIT		0x00000002
#define INTERRUPTSTATUS_TXBUFFER1TRANSMIT		0x00000004
#define INTERRUPTSTATUS_TXBUFFER2TRANSMIT		0x00000008
#define INTERRUPTSTATUS_ERRORWARNING			0x00000010
#define INTERRUPTSTATUS_ERRORPASSIVE			0x00000020
#define INTERRUPTSTATUS_BUSOFF				0x00000040
#define INTERRUPTSTATUS_ARBITRATIONLOST			0x00000080
#define INTERRUPTSTATUS_RXBUSERROR			0x00000100
#define INTERRUPTSTATUS_RXFIFOOVERRUN			0x00000200
#define INTERRUPTSTATUS_POWERSAVINGEXIT			0x00000400
#define INTERRUPTSTATUS_BUSERRORDIRECTION		0x00010000

#define INTERRUPTSTATUS_RECEIVE_SHIFT			(0)
#define INTERRUPTSTATUS_TXBUFFER0TRANSMIT_SHIFT		(1)
#define INTERRUPTSTATUS_TXBUFFER1TRANSMIT_SHIFT		(2)
#define INTERRUPTSTATUS_TXBUFFER2TRANSMIT_SHIFT		(3)
#define INTERRUPTSTATUS_ERRORWARNING_SHIFT		(4)
#define INTERRUPTSTATUS_ERRORPASSIVE_SHIFT		(5)
#define INTERRUPTSTATUS_BUSOFF_SHIFT			(6)
#define INTERRUPTSTATUS_ARBITRATIONLOST_SHIFT		(7)
#define INTERRUPTSTATUS_RXBUSERROR_SHIFT		(8)
#define INTERRUPTSTATUS_RXFIFOOVERRUN_SHIFT		(9)
#define INTERRUPTSTATUS_POWERSAVINGEXIT_SHIFT		(10)
#define INTERRUPTSTATUS_BUSERRORDIRECTION_SHIFT		(16)

#define INTERRUPTSTATUS_RECEIVE_RANGE			0x00000001
#define INTERRUPTSTATUS_TXBUFFER0TRANSMIT_RANGE		0x00000001
#define INTERRUPTSTATUS_TXBUFFER1TRANSMIT_RANGE		0x00000001
#define INTERRUPTSTATUS_TXBUFFER2TRANSMIT_RANGE		0x00000001
#define INTERRUPTSTATUS_ERRORWARNING_RANGE		0x00000001
#define INTERRUPTSTATUS_ERRORPASSIVE_RANGE		0x00000001
#define INTERRUPTSTATUS_BUSOFF_RANGE			0x00000001
#define INTERRUPTSTATUS_ARBITRATIONLOST_RANGE		0x00000001
#define INTERRUPTSTATUS_RXBUSERROR_RANGE		0x00000001
#define INTERRUPTSTATUS_RXFIFOOVERRUN_RANGE		0x00000001
#define INTERRUPTSTATUS_POWERSAVINGEXIT_RANGE		0x00000001
#define INTERRUPTSTATUS_BUSERRORDIRECTION_RANGE		0x00000001

#define BUSERRORDIRECTION_DURINGTRANSMITTING	0
#define BUSERRORDIRECTION_DURINGRECEIVING	1

#define GLOBALSTATUS_RECEIVINGMESSAGE			0x00000001
#define GLOBALSTATUS_TRANSMITTINGMESSAGE		0x00000002
#define GLOBALSTATUS_BUSOFF				0x00000004
#define GLOBALSTATUS_POWERSAVING			0x00000008
#define GLOBALSTATUS_ERRORPASSIVE			0x00000010
#define GLOBALSTATUS_ERRORWARNING		0x00000020
#define GLOBALSTATUS_CONTROLLERSTATE			0x00000700

#define GLOBALSTATUS_RECEIVINGMESSAGE_SHIFT		(0)
#define GLOBALSTATUS_TRANSMITTINGMESSAGE_SHIFT		(1)
#define GLOBALSTATUS_BUSOFF_SHIFT			(2)
#define GLOBALSTATUS_POWERSAVING_SHIFT		(3)
#define GLOBALSTATUS_ERRORPASSIVE_SHIFT		(4)
#define GLOBALSTATUS_ERRORWARNING_SHIFT		(5)
#define GLOBALSTATUS_CONTROLLERSTATE_SHIFT		(8)

#define GLOBALSTATUS_RECEIVINGMESSAGE_RANGE		0x00000001
#define GLOBALSTATUS_TRANSMITTINGMESSAGE_RANGE		0x00000001
#define GLOBALSTATUS_BUSOFF_RANGE			0x00000001
#define GLOBALSTATUS_POWERSAVING_RANGE		0x00000001
#define GLOBALSTATUS_ERRORPASSIVE_RANGE		0x00000001
#define GLOBALSTATUS_ERRORWARNING_RANGE		0x00000001
#define GLOBALSTATUS_CONTROLLERSTATE_RANGE		0x00000007

#define REQUEST_TRANSMITTXBUFFER0		0x00000001
#define REQUEST_ABORTTXBUFFER0			0x00000002
#define REQUEST_TRANSMITTXBUFFER1		0x00000004
#define REQUEST_ABORTTXBUFFER1			0x00000008
#define REQUEST_TRANSMITTXBUFFER2		0x00000010
#define REQUEST_ABORTTXBUFFER2			0x00000020
#define REQUEST_RELEASERECEIVEDMESSAGE		0x00000100

#define REQUEST_TRANSMITTXBUFFER0_SHIFT		(0)
#define REQUEST_ABORTTXBUFFER0_SHIFT		(1)
#define REQUEST_TRANSMITTXBUFFER1_SHIFT		(2)
#define REQUEST_ABORTTXBUFFER1_SHIFT		(3)
#define REQUEST_TRANSMITTXBUFFER2_SHIFT		(4)
#define REQUEST_ABORTTXBUFFER2_SHIFT		(5)
#define REQUEST_RELEASERECEIVEDMESSAGE_SHIFT	(8)

#define REQUEST_TRANSMITTXBUFFER0_RANGE		0x00000001
#define REQUEST_ABORTTXBUFFER0_RANGE		0x00000001
#define REQUEST_TRANSMITTXBUFFER1_RANGE		0x00000001
#define REQUEST_ABORTTXBUFFER1_RANGE		0x00000001
#define REQUEST_TRANSMITTXBUFFER2_RANGE		0x00000001
#define REQUEST_ABORTTXBUFFER2_RANGE		0x00000001
#define REQUEST_RELEASERECEIVEDMESSAGE_RANGE	0x00000001

#define TXBUFFERSTATUS_REQUESTCOMPLETED			0x00000001
#define TXBUFFERSTATUS_REQUESTABORTED			0x00000002
#define TXBUFFERSTATUS_TRANSMITCOMPLETE			0x00000004
#define TXBUFFERSTATUS_ARBITRATIONLOST			0x00000010
#define TXBUFFERSTATUS_BUSERROR				0x00000020
#define TXBUFFERSTATUS_BUSOFF				0x00000040
#define TXBUFFERSTATUS_ARBITLOSTLOCATION		0x00001F00
#define TXBUFFERSTATUS_BUSERRORTYPE			0x00070000
#define TXBUFFERSTATUS_BUSERRORLOCATION			0x1F000000

#define TXBUFFERSTATUS_REQUESTCOMPLETED_SHIFT		(0)
#define TXBUFFERSTATUS_REQUESTABORTED_SHIFT		(1)
#define TXBUFFERSTATUS_TRANSMITCOMPLETE_SHIFT		(2)
#define TXBUFFERSTATUS_ARBITRATIONLOST_SHIFT		(4)
#define TXBUFFERSTATUS_BUSERROR_SHIFT			(5)
#define TXBUFFERSTATUS_BUSOFF_SHIFT			(6)
#define TXBUFFERSTATUS_ARBITLOSTLOCATION_SHIFT		(8)
#define TXBUFFERSTATUS_BUSERRORTYPE_SHIFT		(16)
#define TXBUFFERSTATUS_BUSERRORLOCATION_SHIFT		(24)

#define TXBUFFERSTATUS_REQUESTCOMPLETED_RANGE		0x00000001
#define TXBUFFERSTATUS_REQUESTABORTED_RANGE		0x00000001
#define TXBUFFERSTATUS_TRANSMITCOMPLETE_RANGE		0x00000001
#define TXBUFFERSTATUS_ARBITRATIONLOST_RANGE		0x00000001
#define TXBUFFERSTATUS_BUSERROR_RANGE			0x00000001
#define TXBUFFERSTATUS_BUSOFF_RANGE			0x00000001
#define TXBUFFERSTATUS_ARBITLOSTLOCATION_RANGE		0x0000001F
#define TXBUFFERSTATUS_BUSERRORTYPE_RANGE		0x00000007
#define TXBUFFERSTATUS_BUSERRORLOCATION_RANGE		0x0000001F

#define RECEIVESTATUS_FIFOPENDING			0x00000001
#define RECEIVESTATUS_FIFOOVERRUN			0x00000002
#define RECEIVESTATUS_BUSERROR				0x00000004
#define RECEIVESTATUS_BUSERRORTYPE			0x00000070
#define RECEIVESTATUS_BUSERRORLOCATION			0x00001F00

#define RECEIVESTATUS_FIFOPENDING_SHIFT			(0)
#define RECEIVESTATUS_FIFOOVERRUN_SHIFT			(1)
#define RECEIVESTATUS_BUSERROR_SHIFT			(2)
#define RECEIVESTATUS_BUSERRORTYPE_SHIFT		(4)
#define RECEIVESTATUS_BUSERRORLOCATION_SHIFT		(8)

#define RECEIVESTATUS_FIFOPENDING_RANGE			0x00000001
#define RECEIVESTATUS_FIFOOVERRUN_RANGE			0x00000001
#define RECEIVESTATUS_BUSERROR_RANGE			0x00000001
#define RECEIVESTATUS_BUSERRORTYPE_RANGE		0x00000007
#define RECEIVESTATUS_BUSERRORLOCATION_RANGE		0x0000001F

#define ERRORWARNINGLIMIT_WARNINGLIMIT		0x000000FF

#define ERRORWARNINGLIMIT_WARNINGLIMIT_SHIFT	(0)

#define ERRORWARNINGLIMIT_WARNINGLIMIT_RANGE	0x000000FF

#define ERRORCOUNTER_TXERRORCOUNTER		0x000001FF
#define ERRORCOUNTER_RXERRORCOUNTER		0x01FF0000

#define ERRORCOUNTER_TXERRORCOUNTER_SHIFT	(0)
#define ERRORCOUNTER_RXERRORCOUNTER_SHIFT	(16)

#define ERRORCOUNTER_TXERRORCOUNTER_RANGE	0x000001FF
#define ERRORCOUNTER_RXERRORCOUNTER_RANGE	0x000001FF

#define IDINDEX_FILTERINDEX		0x0000001F
#define IDINDEX_FILTERUPDATE		0x00000100

#define IDINDEX_FILTERINDEX_SHIFT	(0)
#define IDINDEX_FILTERUPDATE_SHIFT	(8)

#define IDINDEX_FILTERINDEX_RANGE	0x0000001F
#define IDINDEX_FILTERUPDATE_RANGE	0x00000001

#define IDFILTER_ID			0x1FFFFFFF
#define IDFILTER_EXTENDEDID		0x40000000
#define IDFILTER_ENABLE			0x80000000

#define IDFILTER_ID_SHIFT		(0)
#define IDFILTER_EXTENDEDID_SHIFT	(30)
#define IDFILTER_ENABLE_SHIFT		(31)

#define IDFILTER_ID_RANGE		0x1FFFFFFF
#define IDFILTER_EXTENDEDID_RANGE	0x00000001
#define IDFILTER_ENABLE_RANGE		0x00000001

#define IDMASK_MASK			0x1FFFFFFF

#define IDMASK_MASK_SHIFT		(0)

#define IDMASK_MASK_RANGE		0x1FFFFFFF

#define TFI_EXTENDEDFRAME			0x00000001
#define TFI_RTRBIT				0x00000002
#define TFI_DATALENGTHCODE			0x000000F0

#define TFI_EXTENDEDFRAME_SHIFT			(0)
#define TFI_RTRBIT_SHIFT			(1)
#define TFI_DATALENGTHCODE_SHIFT		(4)

#define TFI_EXTENDEDFRAME_RANGE			0x00000001
#define TFI_RTRBIT_RANGE			0x00000001
#define TFI_DATALENGTHCODE_RANGE		0x0000000F

#define TID_STANDARDID		0x000007FF
#define TID_EXTENDEDID		0x1FFFFFFF

#define TID_STANDARDID_SHIFT	(0)
#define TID_EXTENDEDID_SHIFT	(0)

#define TID_STANDARDID_RANGE	0x000007FF
#define TID_EXTENDEDID_RANGE	0x1FFFFFFF

#define RFI_EXTENDEDFRAME		0x00000001
#define RFI_RTRBIT			0x00000002
#define RFI_DATALENGTHCODE		0x000000F0

#define RFI_EXTENDEDFRAME_SHIFT		(0)
#define RFI_RTRBIT_SHIFT		(1)
#define RFI_DATALENGTHCODE_SHIFT	(4)

#define RFI_EXTENDEDFRAME_RANGE		0x00000001
#define RFI_RTRBIT_RANGE		0x00000001
#define RFI_DATALENGTHCODE_RANGE	0x0000000F

#define RID_STANDARDID		0x000007FF
#define RID_EXTENDEDID		0x1FFFFFFF

#define RID_STANDARDID_SHIFT	(0)
#define RID_EXTENDEDID_SHIFT	(0)

#define RID_STANDARDID_RANGE	0x000007FF
#define RID_EXTENDEDID_RANGE	0x1FFFFFFF

/*
 * ----
 * layer hardware driver
 */
/* lz_ layer zero */
static u32 lz_read_reg32(void __iomem *reg_base, u32 reg)
{
	/* ioread8, ioread16, ioread32 */
	/* inb, inw, inl */
	return inl((unsigned long)reg_base + reg);
}

static void lz_write_reg32(void __iomem *reg_base, u32 reg, u32 val)
{
	/* iowrite8, iowrite16, iowrite32 */
	/* outb, outw, outl */
	outl(val, (unsigned long)reg_base + reg);
}

struct canglobalconf {
	u32	silent;
	u32	loopback;
	u32	transmitwithnoack;
	u32	selfreception;
	u32	transmitpriority;/* 0: buffer index, 1: id, 2: round robin*/
	u32	arbitrationlostretry;
	u32	buserrorretry;
	u32	special1;
	u32	powersaving;
	u32	rxbuserror;
};

struct canglobalstatus {
	u32	errorwarning;
	u32	errorpassive;
	u32	busoff;
};

struct caninterrupt {
	u32	receive;
	u32	txbuffer0transmit;
	u32	txbuffer1transmit;
	u32	txbuffer2transmit;
	u32	errorwarning;
	u32	errorpassive;
	u32	busoff;
	u32	arbitrationlost;
	u32	rxbuserror;
	u32	rxfifooverrun;
	u32	powersavingexit;
	/*info*/
	u32	buserrordirection;
};

struct cantransmitstatus {
	u32	requestcompleted;
	u32	requestaborted;
	u32	transmitcomplete;
	u32	arbitrationlost;
	u32	buserror;
	u32	busoff;
	u32	arbitlostlocation;
	u32	buserrortype;
	u32	buserrorlocation;
};

struct canreceivestatus {
	u32	fifopending;
	u32	fifooverrun;
	u32	buserror;
	u32	buserrortype;
	u32	buserrorlocation;
};

struct canbuffer {
	u32	extendedframe;
	u32	id;
	u32	rtrbit;
	u32	datalengthcode;
	u32	byte0_3;
	u32	byte4_7;
};

#define CANVOLATILEINFO_MAXMESSAGES	32
struct canvolatileinfo {
	/* in general, a interrupt will get 1 or 2 messages*/
	u32	numrxmessages;
	struct canbuffer	rxbuffer[CANVOLATILEINFO_MAXMESSAGES];
	struct canreceivestatus	sreceivestatus;
	u32	txerrorcounter;
	u32	rxerrorcounter;

	u32	dummy;
};

struct hwd_can {
	u32	portid;
	void __iomem	*reg_base;/* mapped mem or io address*/

	/* Driver Core Data */
};

static void hwdcan_initialize0(
	IN struct hwd_can *this)
{
	/* int retval; return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	this->reg_base = NULL;
}

static void hwdcan_setmappedaddress(
	IN struct hwd_can *this,
	IN void __iomem *reg_base)
{
	this->reg_base = reg_base;
}

static void hwdcan_opendevice(
	IN struct hwd_can *this,
	IN u32 portid)
{
	this->portid = portid;
}

static void hwdcan_closedevice(
	IN struct hwd_can *this)
{
}

static int hwdcan_inithw(
	IN struct hwd_can *this)
{
	struct timespec64 start;
	struct timespec64 end;
	u32 ms;

	u32 reg_globalcontrol;
	u32 reset;

	reg_globalcontrol = lz_read_reg32(
		this->reg_base, LZ_CANREGOFFSET_GLOBALCONTROL);
	_setdatafield(reg_globalcontrol, GLOBALCONTROL_RESET, 1);
	lz_write_reg32(this->reg_base,
		LZ_CANREGOFFSET_GLOBALCONTROL, reg_globalcontrol);
	/* wait for the action done. */
	ktime_get_real_ts64(&start);
	while (1) {
		reg_globalcontrol = lz_read_reg32(
			this->reg_base, LZ_CANREGOFFSET_GLOBALCONTROL);
		_getdatafield(reg_globalcontrol, GLOBALCONTROL_RESET, reset);
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
	return 0;
}

static int hwdcan_isr(
	IN struct hwd_can *this,
	OUT u32 *pbinterrupt,
	OUT struct caninterrupt *pinterrupt,
	OUT struct canvolatileinfo  *pvolatileinfo)
{
	int retval; /* return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	u32 reg_totalinterruptstatus;
	u32 reg_interruptstatus;
	struct caninterrupt sinterruptstatus;
	u32 reg_RFI;
	u32 reg_RID;
	u32 reg_RDL;
	u32 reg_RDH;
	u32 reg_request;
	u32 reg_recstatus;
	u32 reg_errorcounter;
	u32 pending;
	struct canbuffer *pcanbuffer;

	retval = 0;
	reg_totalinterruptstatus = 0;
	reg_recstatus = 0;

	*pbinterrupt = FALSE;
	while (1) {
		reg_interruptstatus = lz_read_reg32(
			this->reg_base, LZ_CANREGOFFSET_INTERRUPTSTATUS);
		/* to quickly handle interrupts. */
		/* reg_interruptstatus & STATUS_INTERRUPT_MASK */
		if ((reg_interruptstatus & STATUS_INTERRUPT_MASK) == 0)
			break;
		*pbinterrupt = TRUE;
		pending = 1;
		while ((reg_interruptstatus & INTERRUPTSTATUS_RECEIVE) &&
			pending != 0) {
			reg_RFI = lz_read_reg32(
				this->reg_base, LZ_CANREGOFFSET_RFI);
			reg_RID = lz_read_reg32(
				this->reg_base, LZ_CANREGOFFSET_RID);
			reg_RDL = lz_read_reg32(
				this->reg_base, LZ_CANREGOFFSET_RDL);
			reg_RDH = lz_read_reg32(
				this->reg_base, LZ_CANREGOFFSET_RDH);
			reg_request = REQUEST_RELEASERECEIVEDMESSAGE;
			lz_write_reg32(this->reg_base,
				LZ_CANREGOFFSET_REQUEST, reg_request);
			pcanbuffer = &pvolatileinfo->rxbuffer[
				pvolatileinfo->numrxmessages];
			_getdatafield(reg_RFI, RFI_EXTENDEDFRAME,
				pcanbuffer->extendedframe);
			_getdatafield(reg_RFI, RFI_RTRBIT,
				pcanbuffer->rtrbit);
			_getdatafield(reg_RFI, RFI_DATALENGTHCODE,
				pcanbuffer->datalengthcode);
			pcanbuffer->id = reg_RID & RID_STANDARDID;
			if (pcanbuffer->extendedframe)
				pcanbuffer->id = reg_RID & RID_EXTENDEDID;
			pcanbuffer->byte0_3 = reg_RDL;
			pcanbuffer->byte4_7 = reg_RDH;
			pvolatileinfo->numrxmessages += 1;
			reg_recstatus = lz_read_reg32(this->reg_base,
				LZ_CANREGOFFSET_RECEIVESTATUS);
			pending = 0;
			if (reg_recstatus & RECEIVESTATUS_FIFOPENDING)
				pending = 1;
			if (pvolatileinfo->numrxmessages ==
				CANVOLATILEINFO_MAXMESSAGES)
				break;
		}
		/* quickly clear interrupt that are handled. */
		reg_totalinterruptstatus |= reg_interruptstatus;
		lz_write_reg32(this->reg_base,
			LZ_CANREGOFFSET_INTERRUPTSTATUS, reg_interruptstatus);
	}
	reg_interruptstatus = reg_totalinterruptstatus;
	if (reg_interruptstatus == 0)
		goto funcexit;

	if (reg_interruptstatus &
		(INTERRUPTSTATUS_ERRORWARNING | INTERRUPTSTATUS_ERRORPASSIVE)) {
		reg_errorcounter = lz_read_reg32(
			this->reg_base, LZ_CANREGOFFSET_ERRORCOUNTER);
		_getdatafield(reg_errorcounter, ERRORCOUNTER_TXERRORCOUNTER,
			pvolatileinfo->txerrorcounter);
		_getdatafield(reg_errorcounter, ERRORCOUNTER_RXERRORCOUNTER,
			pvolatileinfo->rxerrorcounter);
	}
	_getdatafield(reg_interruptstatus, INTERRUPTSTATUS_RECEIVE,
		sinterruptstatus.receive);
	_getdatafield(reg_interruptstatus, INTERRUPTSTATUS_TXBUFFER0TRANSMIT,
		sinterruptstatus.txbuffer0transmit);
	_getdatafield(reg_interruptstatus, INTERRUPTSTATUS_TXBUFFER1TRANSMIT,
		sinterruptstatus.txbuffer1transmit);
	_getdatafield(reg_interruptstatus, INTERRUPTSTATUS_TXBUFFER2TRANSMIT,
		sinterruptstatus.txbuffer2transmit);
	_getdatafield(reg_interruptstatus, INTERRUPTSTATUS_ERRORWARNING,
		sinterruptstatus.errorwarning);
	_getdatafield(reg_interruptstatus, INTERRUPTSTATUS_ERRORPASSIVE,
		sinterruptstatus.errorpassive);
	_getdatafield(reg_interruptstatus, INTERRUPTSTATUS_BUSOFF,
		sinterruptstatus.busoff);
	_getdatafield(reg_interruptstatus, INTERRUPTSTATUS_ARBITRATIONLOST,
		sinterruptstatus.arbitrationlost);
	_getdatafield(reg_interruptstatus, INTERRUPTSTATUS_RXBUSERROR,
		sinterruptstatus.rxbuserror);
	_getdatafield(reg_interruptstatus, INTERRUPTSTATUS_RXFIFOOVERRUN,
		sinterruptstatus.rxfifooverrun);
	_getdatafield(reg_interruptstatus, INTERRUPTSTATUS_POWERSAVINGEXIT,
		sinterruptstatus.powersavingexit);
	_getdatafield(reg_interruptstatus, INTERRUPTSTATUS_BUSERRORDIRECTION,
		sinterruptstatus.buserrordirection);

	_getdatafield(reg_recstatus, RECEIVESTATUS_FIFOPENDING,
		pvolatileinfo->sreceivestatus.fifopending);
	_getdatafield(reg_recstatus, RECEIVESTATUS_FIFOOVERRUN,
		pvolatileinfo->sreceivestatus.fifooverrun);
	_getdatafield(reg_recstatus, RECEIVESTATUS_BUSERROR,
		pvolatileinfo->sreceivestatus.buserror);
	_getdatafield(reg_recstatus, RECEIVESTATUS_BUSERRORTYPE,
		pvolatileinfo->sreceivestatus.buserrortype);
	_getdatafield(reg_recstatus, RECEIVESTATUS_BUSERRORLOCATION,
		pvolatileinfo->sreceivestatus.buserrorlocation);

	*pinterrupt = sinterruptstatus;

	goto funcexit;
funcexit:

	return retval;
}

/*static int hwdcan_dpcforisr(
 *	IN struct hwd_can *this,
 *	IN struct caninterrupt *pinterrupt,
 *	IN struct canvolatileinfo  *pvolatileinfo)
 *{
 *	return 0;
 *}
 */

static void hwdcan_enableinterrupt(
	IN struct hwd_can *this,
	IN struct caninterrupt *pinterrupt)
{
	u32 reg_intenable;

	reg_intenable = lz_read_reg32(
		this->reg_base, LZ_CANREGOFFSET_INTERRUPTENABLE);
	if (pinterrupt->receive)
		_setdatafield(reg_intenable, INTERRUPTENABLE_RECEIVE, 1);
	if (pinterrupt->txbuffer0transmit)
		_setdatafield(reg_intenable,
			INTERRUPTENABLE_TXBUFFER0TRANSMIT, 1);
	if (pinterrupt->txbuffer1transmit)
		_setdatafield(reg_intenable,
			INTERRUPTENABLE_TXBUFFER1TRANSMIT, 1);
	if (pinterrupt->txbuffer2transmit)
		_setdatafield(reg_intenable,
			INTERRUPTENABLE_TXBUFFER2TRANSMIT, 1);
	if (pinterrupt->errorwarning)
		_setdatafield(reg_intenable, INTERRUPTENABLE_ERRORWARNING, 1);
	if (pinterrupt->errorpassive)
		_setdatafield(reg_intenable, INTERRUPTENABLE_ERRORPASSIVE, 1);
	if (pinterrupt->busoff)
		_setdatafield(reg_intenable, INTERRUPTENABLE_BUSOFF, 1);
	if (pinterrupt->arbitrationlost)
		_setdatafield(reg_intenable,
			INTERRUPTENABLE_ARBITRATIONLOST, 1);
	if (pinterrupt->rxbuserror)
		_setdatafield(reg_intenable, INTERRUPTENABLE_RXBUSERROR, 1);
	if (pinterrupt->rxfifooverrun)
		_setdatafield(reg_intenable, INTERRUPTENABLE_RXFIFOOVERRUN, 1);
	if (pinterrupt->powersavingexit)
		_setdatafield(reg_intenable,
			INTERRUPTENABLE_POWERSAVINGEXIT, 1);
	lz_write_reg32(this->reg_base,
		LZ_CANREGOFFSET_INTERRUPTENABLE, reg_intenable);
}

static void hwdcan_disableinterrupt(
	IN struct hwd_can *this,
	IN struct caninterrupt *pinterrupt)
{
	u32 reg_intenable;

	reg_intenable = lz_read_reg32(
		this->reg_base, LZ_CANREGOFFSET_INTERRUPTENABLE);
	if (pinterrupt->receive)
		_setdatafield(reg_intenable, INTERRUPTENABLE_RECEIVE, 0);
	if (pinterrupt->txbuffer0transmit)
		_setdatafield(reg_intenable,
			INTERRUPTENABLE_TXBUFFER0TRANSMIT, 0);
	if (pinterrupt->txbuffer1transmit)
		_setdatafield(reg_intenable,
			INTERRUPTENABLE_TXBUFFER1TRANSMIT, 0);
	if (pinterrupt->txbuffer2transmit)
		_setdatafield(reg_intenable,
			INTERRUPTENABLE_TXBUFFER2TRANSMIT, 0);
	if (pinterrupt->errorwarning)
		_setdatafield(reg_intenable, INTERRUPTENABLE_ERRORWARNING, 0);
	if (pinterrupt->errorpassive)
		_setdatafield(reg_intenable, INTERRUPTENABLE_ERRORPASSIVE, 0);
	if (pinterrupt->busoff)
		_setdatafield(reg_intenable, INTERRUPTENABLE_BUSOFF, 0);
	if (pinterrupt->arbitrationlost)
		_setdatafield(reg_intenable,
			INTERRUPTENABLE_ARBITRATIONLOST, 0);
	if (pinterrupt->rxbuserror)
		_setdatafield(reg_intenable, INTERRUPTENABLE_RXBUSERROR, 0);
	if (pinterrupt->rxfifooverrun)
		_setdatafield(reg_intenable, INTERRUPTENABLE_RXFIFOOVERRUN, 0);
	if (pinterrupt->powersavingexit)
		_setdatafield(reg_intenable,
			INTERRUPTENABLE_POWERSAVINGEXIT, 0);
	lz_write_reg32(this->reg_base,
		LZ_CANREGOFFSET_INTERRUPTENABLE, reg_intenable);
}

static void hwdcan_setglobalconfigure(
	IN struct hwd_can *this,
	IN struct canglobalconf *pglobalconf)
{
	u32 reg_globalcontrol;

	reg_globalcontrol = lz_read_reg32(
		this->reg_base, LZ_CANREGOFFSET_GLOBALCONTROL);
	_setdatafield(reg_globalcontrol, GLOBALCONTROL_SILENT,
		pglobalconf->silent);
	_setdatafield(reg_globalcontrol, GLOBALCONTROL_LOOPBACK,
		pglobalconf->loopback);
	_setdatafield(reg_globalcontrol, GLOBALCONTROL_TRANSMITWITHNOACK,
		pglobalconf->transmitwithnoack);
	_setdatafield(reg_globalcontrol, GLOBALCONTROL_SELFRECEPTION,
		pglobalconf->selfreception);
	_setdatafield(reg_globalcontrol, GLOBALCONTROL_TRANSMITPRIORITY,
		pglobalconf->transmitpriority);
	_setdatafield(reg_globalcontrol, GLOBALCONTROL_ARBITRATIONLOSTRETRY,
		pglobalconf->arbitrationlostretry);
	_setdatafield(reg_globalcontrol, GLOBALCONTROL_BUSERRORRETRY,
		pglobalconf->buserrorretry);
	_setdatafield(reg_globalcontrol, GLOBALCONTROL_SPECIAL1,
		pglobalconf->special1);
	_setdatafield(reg_globalcontrol, GLOBALCONTROL_IDENTIFIERENABLE, 1);
	_setdatafield(reg_globalcontrol, GLOBALCONTROL_IDENTIFIERBYPASS, 1);
	_setdatafield(reg_globalcontrol, GLOBALCONTROL_POWERSAVING,
		pglobalconf->powersaving);
	_setdatafield(reg_globalcontrol, GLOBALCONTROL_RXBUSERROR,
		pglobalconf->rxbuserror);
	lz_write_reg32(this->reg_base,
		LZ_CANREGOFFSET_GLOBALCONTROL, reg_globalcontrol);
}

static void hwdcan_setclockprescale(
	IN struct hwd_can *this,
	IN u32 ckdiv)
{
	u32 reg_prescale;

	reg_prescale = ckdiv | CLOCKPRESCALER_EXTERNALINPUT;
	lz_write_reg32(
		this->reg_base, LZ_CANREGOFFSET_CLOCKPRESCALER, reg_prescale);
}

static void hwdcan_setbustiming(
	IN struct hwd_can *this,
	IN u32 propseg,
	IN u32 pseg1,
	IN u32 pseg2,
	IN u32 sjw,
	IN u32 sampling)
{
	u32 reg_bustiming;

	reg_bustiming = 0;
	_setdatafield(reg_bustiming, BUSTIMING_PROPAGATIONSEG, propseg);
	_setdatafield(reg_bustiming, BUSTIMING_PHASESEG1, pseg1);
	_setdatafield(reg_bustiming, BUSTIMING_PHASESEG2, pseg2);
	_setdatafield(reg_bustiming, BUSTIMING_SJW, sjw);
	_setdatafield(reg_bustiming, BUSTIMING_SAMPLING, sampling);
	lz_write_reg32(
		this->reg_base, LZ_CANREGOFFSET_BUSTIMING, reg_bustiming);
}

static void hwdcan_hardreset(
	IN struct hwd_can *this)
{
	hwdcan_inithw(this);
}

static void _hwdcan_setactive(
	IN struct hwd_can *this,
	IN u32 active)
{
	struct timespec64 start;
	struct timespec64 end;
	u32 ms;

	u32 reg_globalcontrol;
	u32 temp;

	reg_globalcontrol = lz_read_reg32(
		this->reg_base, LZ_CANREGOFFSET_GLOBALCONTROL);
	_setdatafield(reg_globalcontrol, GLOBALCONTROL_ACTIVE, active);
	lz_write_reg32(this->reg_base,
		LZ_CANREGOFFSET_GLOBALCONTROL, reg_globalcontrol);
	/* wait for the action done. */
	ktime_get_real_ts64(&start);
	while (1) {
		reg_globalcontrol = lz_read_reg32(
			this->reg_base, LZ_CANREGOFFSET_GLOBALCONTROL);
		_getdatafield(reg_globalcontrol, GLOBALCONTROL_ACTIVE, temp);
		if (temp == active)
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

static void hwdcan_start(
	IN struct hwd_can *this)
{
	_hwdcan_setactive(this, 1);
}

static void hwdcan_stop(
	IN struct hwd_can *this)
{
	_hwdcan_setactive(this, 0);
}

static void hwdcan_getglobalstatus(
	IN struct hwd_can *this,
	OUT struct canglobalstatus *pglobalstatus)
{
	u32 reg_globalstatus;

	reg_globalstatus = lz_read_reg32(
		this->reg_base, LZ_CANREGOFFSET_GLOBALSTATUS);
	_getdatafield(reg_globalstatus, GLOBALSTATUS_BUSOFF,
		pglobalstatus->busoff);
	_getdatafield(reg_globalstatus, GLOBALSTATUS_ERRORPASSIVE,
		pglobalstatus->errorpassive);
	_getdatafield(reg_globalstatus, GLOBALSTATUS_ERRORWARNING,
		pglobalstatus->errorwarning);
}

static void hwdcan_gettransmitstatus(
	IN struct hwd_can *this,
	IN u32 index,
	OUT struct cantransmitstatus *ptransmitstatus)
{
	u32 reg_txbufferstatus;
	u32 regoffset;

	switch (index) {
	case 1:
		regoffset = LZ_CANREGOFFSET_TXBUFFER1STATUS;
		break;
	case 2:
		regoffset = LZ_CANREGOFFSET_TXBUFFER2STATUS;
		break;
	case 0:
	default:
		regoffset = LZ_CANREGOFFSET_TXBUFFER0STATUS;
		break;
	}

	reg_txbufferstatus = lz_read_reg32(this->reg_base, regoffset);
	_getdatafield(reg_txbufferstatus, TXBUFFERSTATUS_REQUESTCOMPLETED,
		ptransmitstatus->requestcompleted);
	_getdatafield(reg_txbufferstatus, TXBUFFERSTATUS_REQUESTABORTED,
		ptransmitstatus->requestaborted);
	_getdatafield(reg_txbufferstatus, TXBUFFERSTATUS_TRANSMITCOMPLETE,
		ptransmitstatus->transmitcomplete);
	_getdatafield(reg_txbufferstatus, TXBUFFERSTATUS_ARBITRATIONLOST,
		ptransmitstatus->arbitrationlost);
	_getdatafield(reg_txbufferstatus, TXBUFFERSTATUS_BUSERROR,
		ptransmitstatus->buserror);
	_getdatafield(reg_txbufferstatus, TXBUFFERSTATUS_BUSOFF,
		ptransmitstatus->busoff);
	_getdatafield(reg_txbufferstatus, TXBUFFERSTATUS_ARBITLOSTLOCATION,
		ptransmitstatus->arbitlostlocation);
	_getdatafield(reg_txbufferstatus, TXBUFFERSTATUS_BUSERRORTYPE,
		ptransmitstatus->buserrortype);
	_getdatafield(reg_txbufferstatus, TXBUFFERSTATUS_BUSERRORLOCATION,
		ptransmitstatus->buserrorlocation);

}

static void hwdcan_geterrorcounter(
	IN struct hwd_can *this,
	OUT u32 *ptxerrorcounter,
	OUT u32 *prxerrorcounter)
{
	u32 reg_errorcounter;

	reg_errorcounter = lz_read_reg32(
		this->reg_base, LZ_CANREGOFFSET_ERRORCOUNTER);
	_getdatafield(reg_errorcounter, ERRORCOUNTER_TXERRORCOUNTER,
		(*ptxerrorcounter));
	_getdatafield(reg_errorcounter, ERRORCOUNTER_RXERRORCOUNTER,
		(*prxerrorcounter));
}

static void hwdcan_sendtxbuffer(
	IN struct hwd_can *this,
	IN u32 index,
	OUT struct canbuffer *pcanbuffer)
{
	u32 reg_TFI;
	u32 reg_TID;
	u32 reg_TDL;
	u32 reg_TDH;
	u32 reg_request;
	u32 regoffset;

	switch (index) {
	case 1:
		regoffset = LZ_CANREGOFFSET_TFI1;
		reg_request = REQUEST_TRANSMITTXBUFFER1;
		break;
	case 2:
		regoffset = LZ_CANREGOFFSET_TFI2;
		reg_request = REQUEST_TRANSMITTXBUFFER2;
		break;
	case 0:
	default:
		regoffset = LZ_CANREGOFFSET_TFI0;
		reg_request = REQUEST_TRANSMITTXBUFFER0;
		break;
	}

	reg_TFI = 0;
	_setdatafield(reg_TFI, TFI_EXTENDEDFRAME, pcanbuffer->extendedframe);
	_setdatafield(reg_TFI, TFI_RTRBIT, pcanbuffer->rtrbit);
	_setdatafield(reg_TFI, TFI_DATALENGTHCODE, pcanbuffer->datalengthcode);
	reg_TID = pcanbuffer->id & RID_STANDARDID;
	if (pcanbuffer->extendedframe)
		reg_TID = pcanbuffer->id & RID_EXTENDEDID;
	reg_TDL = pcanbuffer->byte0_3;
	reg_TDH = pcanbuffer->byte4_7;
	lz_write_reg32(this->reg_base, regoffset, reg_TFI);
	lz_write_reg32(this->reg_base, regoffset+4, reg_TID);
	lz_write_reg32(this->reg_base, regoffset+8, reg_TDL);
	lz_write_reg32(this->reg_base, regoffset+12, reg_TDH);
	lz_write_reg32(this->reg_base, LZ_CANREGOFFSET_REQUEST, reg_request);
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

#define INTERRUPTQUEUE_MAXCOUNT		128
struct interrupt_extension {
	/*
	 * DpcQueue;
	 * Queue size depend on the frequency of interrupt.
	 * Dont let the queue be full
	 */

	struct caninterrupt	losinterrupt[INTERRUPTQUEUE_MAXCOUNT];
	struct canvolatileinfo	losvolatileinfo[INTERRUPTQUEUE_MAXCOUNT];
	u32	start;
	u32	end;
	u32	count;
	u32	overrun;
	u32	id; /* sequential id*/

	u32	dummy;
};

#define CAN_NUMBER_TXBUFFER	3
#define CAN_ECHO_SKB_MAX	3/* this controller have 3 tx buffer.*/

struct this_canpriv {
	/* alloc_candev need can_priv*/
	struct can_priv		can;
		/* fixed, shared with can_priv *priv */
	struct napi_struct	napi;/* for napi polling */
	int			moduleportindex;
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
	struct net_device	*candev;/* alloc_candev */
		/*container_of(adapter, struct module_port_device, sadapter);*/
	struct this_canpriv	*canpriv;
		/* netdev_priv(candev) */
	char			*coredev_name;
		/* dev_name(ThisCoreDevice.dev) */
	/*spinlock_t		spinlock_candev;*/
		/* lock candev function */
	unsigned int		emptytxindex;
	unsigned int		txbufferflag[3];
		/* cb_candev_start_xmit set flag to 1;*/
		/* isrdpc set flag to 0;*/
	unsigned int		txbufferdlc[3];

	/* device capability */
	unsigned int		sourceclock;

	/* monitor device status */

	/* device handles and resources */
	struct hwd_can		shwdcan;
	struct hwd_can		*hwdcan;
	struct caninterrupt	scaninterrupt;

	struct canglobalconf	scanglobalconf;

	struct list_head	shared_irq_entry;
		/* shared_irq_entry will link all IRQs this module used.*/

	/*struct timer_list	timer_noirq;*/

	struct tasklet_struct	tasklet_isrdpc;

	/* Synchronization */

	/* ISRDPC Queue and Rountine */
	spinlock_t		interruptlock_irq;
		/* for some operation that will generate interrupt */

	spinlock_t		interruptlock_dpcqueue;
	struct interrupt_extension	sinterrupt_extension;
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

static struct can_bittiming_const this_can_bittiming = {
	.name		= "RDC_CAN",
	.tseg1_min	= 5,/* see SYNC + Prop + Seg1 + Seg2 list table */
	.tseg1_max	= 16,/* timeseg1 = prop_seg + phase_seg1 */
	.tseg2_min	= 3,/* see SYNC + Prop + Seg1 + Seg2 list table */
	.tseg2_max	= 8,/* timeseg2 = phase_seg2 */
	.sjw_max	= 4,
	.brp_min	= 2,/* see our spec. */
	.brp_max	= 0x20000,/* prescaler our spec (0xFFFF + 1)*2 */
	.brp_inc	= 2,/* see our spec. */
};

static int candev_start(struct net_device *candev);
static int candev_stop(struct net_device *candev);
static int _candev_tx_aborted(struct net_device *candev, unsigned int txindex);
static int _candev_tx_error(
	struct net_device *candev,
	unsigned int txindex, struct cantransmitstatus *ptransmitstatus);
static int _candev_tx(
	struct net_device *candev, unsigned int txindex, unsigned int dlc);
static int _candev_rx_error(
	struct net_device *candev, struct canreceivestatus *preceivestatus);
static int _candev_rx_overrun(struct net_device *candev);
static int _candev_rx(struct net_device *candev, struct canbuffer *prxbuffer);
static int _candev_errorwarning(
	struct net_device *candev,
	unsigned int txerrorcounter, unsigned int rxerrorcounter);
static int _candev_errorpassive(
	struct net_device *candev,
	unsigned int txerrorcounter, unsigned int rxerrorcounter);
static int _candev_busoff(struct net_device *candev);
/*static int _candev_arbitlost(struct net_device *candev);*/

static int cb_can_open(struct net_device *candev);
static int cb_can_close(struct net_device *candev);
static int cb_can_start_xmit(
	struct sk_buff *skb, struct net_device *candev);
static int cb_can_start_xmit(struct sk_buff *skb, struct net_device *candev);
static int cb_can_set_bittiming(struct net_device *candev);
static int cb_can_set_mode(struct net_device *candev, enum can_mode canmode);
static int cb_can_get_state(
	const struct net_device *candev, enum can_state *pcanstate);
static int cb_can_get_berr_counter(
	const struct net_device *candev,
	struct can_berr_counter *pcanberrcounter);
static const struct net_device_ops can_netdev_ops = {
	.ndo_open	= cb_can_open,
	.ndo_stop	= cb_can_close,
	.ndo_start_xmit	= cb_can_start_xmit,
	/*.ndo_do_ioctl	= rdc_ndo_do_ioctl, dont sleep. */
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
	this_port_device->candev = NULL;
	this_port_device->canpriv = NULL;
	this_port_device->hwdcan = &this_port_device->shwdcan;
	hwdcan_initialize0(this_port_device->hwdcan);
	this_port_device->emptytxindex = 0;
	this_port_device->txbufferflag[0] = 0;
	this_port_device->txbufferflag[1] = 0;
	this_port_device->txbufferflag[2] = 0;
	/*spin_lock_init(&this_port_device->spinlock_candev);*/

	memset(&this_port_device->scanglobalconf, 0,
		sizeof(struct canglobalconf));

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
	unsigned int sourceclock)
{
	int retval; /* return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	struct module_port_device *this_port_device;

	pr_info("%s Entry moduleportindex:%u\n", __func__, moduleportindex);
	retval = 0;
	this_port_device = &thismodule_port_devices[moduleportindex];
	this_port_device->sourceclock = sourceclock;

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
	struct net_device	*candev;
	struct this_canpriv	*canpriv;

	pr_info("%s Entry moduleportindex:%u\n", __func__, moduleportindex);
	retval = 0;

	pr_info("    dev_name %s\n", dev_name(dev)); /* ex: 0000:03:06.0 */

	/* pcidev = container_of(dev, struct pci_dev, device); */
	this_port_device = &thismodule_port_devices[moduleportindex];
	/* this_driver_data = dev_get_drvdata(dev); */
	/* this_specific = this_driver_data->this_specific; */

	/* allocate ThisCoreDevice */
	candev = alloc_candev(sizeof(struct this_canpriv), CAN_ECHO_SKB_MAX);
	if (candev == NULL) {
		pr_err("    alloc_candev return null\n");
		retval = -ENOMEM;
		goto funcexit;
	}

	/* init ThisCoreDevice */
	candev->flags |= IFF_ECHO;
	candev->netdev_ops = &can_netdev_ops;
	SET_NETDEV_DEV(candev, dev);
	candev->irq = this_port_device->irq;

	canpriv = netdev_priv(candev);
	this_port_device->candev = candev;
	this_port_device->canpriv = canpriv;
	/* init canpriv*/
	canpriv->moduleportindex = moduleportindex;
	canpriv->can.clock.freq = this_port_device->sourceclock;
	canpriv->can.bittiming_const = &this_can_bittiming;
	canpriv->can.ctrlmode_supported = CAN_CTRLMODE_LISTENONLY |
		CAN_CTRLMODE_LOOPBACK | CAN_CTRLMODE_ONE_SHOT;
	canpriv->can.do_set_bittiming = cb_can_set_bittiming;
	canpriv->can.do_set_mode = cb_can_set_mode;
	canpriv->can.do_get_state = cb_can_get_state;
	canpriv->can.do_get_berr_counter = cb_can_get_berr_counter;
	/*register napi*/
	/* upper  level rx packet size = 64*/
	/*netif_napi_add(candev, &canpriv->napi, cb_can_napi_poll, 64);*/
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))	
	netif_napi_add(candev, &canpriv->napi, NULL);
#else	
	netif_napi_add(candev, &canpriv->napi, NULL, 64);
#endif

	/* register ThisCoreDevice */
	result = register_candev(candev);
	if (result != 0) {
		pr_err("    register_candev failed %i\n", result);
		retval = -EFAULT;
		goto funcexit;
	}
	/* now, register is ok */
	this_port_device->coredev_name = (char *)dev_name(&candev->dev);
	pr_info("    register ThisCoreDevice ok, dev_name:%s\n",
		this_port_device->coredev_name);
	if (MAJOR(candev->dev.devt) != 0) {
		this_port_device->major = MAJOR(candev->dev.devt);
		this_port_device->deviceid = MINOR(candev->dev.devt);
		pr_info("    now, this port is registered with major:%u deviceid:%u\n",
			this_port_device->major, this_port_device->deviceid);
	} else {
		this_port_device->major = 0;
		this_port_device->deviceid = moduleportindex;
		pr_info("    this port have no major, so we let deviceid equal to moduleportindex:%u\n",
			this_port_device->deviceid);
	}

	/* create device attributes for ThisCoreDevice */
	reterr = device_create_file(&candev->dev, &dev_attr_drivername);
	if (reterr < 0) {
		pr_err("    device_create_file failed %i\n", reterr);
		unregister_candev(candev);
		retval = -EFAULT;
		goto funcexit;
	}
	/* reterr = device_create_file(dev_attr_others); */

	goto funcexit;
funcexit:

	if (retval != 0) {
		if (candev != NULL) {
			netif_napi_del(&canpriv->napi);
			free_candev(candev);
			this_port_device->candev = NULL;
			this_port_device->canpriv = NULL;
		}
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

	if (this_port_device->candev != NULL) {
		device_remove_file(&this_port_device->candev->dev,
			&dev_attr_drivername);

		/* unregister ThisCoreDevice */
		unregister_candev(this_port_device->candev);

		netif_napi_del(&this_port_device->canpriv->napi);
		free_candev(this_port_device->candev);
		this_port_device->candev = NULL;
		this_port_device->canpriv = NULL;
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

	if (this_port_device->opencount == 1) {
		if (netif_running(this_port_device->candev)) {
			netif_stop_queue(this_port_device->candev);
			netif_device_detach(this_port_device->candev);
			candev_stop(this_port_device->candev);
		}
		/* stop queue */
		/* wait for ISRDPC done ??? */
		/* abort and wait for all transmission */
		/* suspend device */
	}

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
		if (netif_running(this_port_device->candev)) {
			candev_start(this_port_device->candev);
			netif_device_attach(this_port_device->candev);
			netif_start_queue(this_port_device->candev);
		}
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
	int reterr; /* check if (reterr < 0) */
	/* unsigned int result; check if (result != 0) */

	retval = 0;

	/* EvtDevicePrepareHardware */
	hwdcan_setmappedaddress(
		this_port_device->hwdcan, this_port_device->reg_base);

	hwdcan_opendevice(
		this_port_device->hwdcan, this_port_device->portnumber);

	reterr = hwdcan_inithw(this_port_device->hwdcan);
	if (reterr < 0) {
		pr_err("    hwdcan_inithw error:x%X\n", reterr);
		retval = -EFAULT;
		goto funcexit;
	} else {
		/* EvtDeviceD0Entry */
		/* hwd_start() */

		/* EvtDeviceD0EntryPostInterruptsEnabled */
		this_port_device->scaninterrupt.receive = TRUE;
		this_port_device->scaninterrupt.txbuffer0transmit = TRUE;
		this_port_device->scaninterrupt.txbuffer1transmit = TRUE;
		this_port_device->scaninterrupt.txbuffer2transmit = TRUE;
		this_port_device->scaninterrupt.errorwarning = TRUE;
		this_port_device->scaninterrupt.errorpassive = TRUE;
		this_port_device->scaninterrupt.rxfifooverrun = TRUE;
		this_port_device->scaninterrupt.busoff = TRUE;
		this_port_device->scaninterrupt.powersavingexit = FALSE;
		this_port_device->scaninterrupt.arbitrationlost = TRUE;
		this_port_device->scaninterrupt.rxbuserror = TRUE;
		/* hwdcan_enableinterrupt() */
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
	hwdcan_disableinterrupt(
		this_port_device->hwdcan, &this_port_device->scaninterrupt);

	/* EvtDeviceD0Exit */
	/* hwdcan_stop() */
	hwdcan_hardreset(this_port_device->hwdcan);

	/* EvtDeviceReleaseHardware */
	hwdcan_closedevice(this_port_device->hwdcan);

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
	unsigned int bserviced;
	unsigned int start;
	unsigned int end;
	unsigned int count;
	unsigned int binterrupt;
	struct caninterrupt sinterrupt = {0};
	struct canvolatileinfo svolatileinfo = {0};

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
		bserviced = FALSE;
		binterrupt = FALSE;
		pinterrupt_ext = &this_port_device->sinterrupt_extension;

		if (this_port_device->opencount == 0)
			continue;
		/* now, handle isr*/
		memset(&sinterrupt, 0, sizeof(sinterrupt));
		memset(&svolatileinfo, 0, sizeof(svolatileinfo));
		binterrupt = FALSE;
		hwdcan_isr(this_port_device->hwdcan,
			&binterrupt, &sinterrupt, &svolatileinfo);
		if (binterrupt == FALSE)
			continue;

		/* pr_info("    binterrupt == TRUE\n"); */
		bserviced = TRUE;

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

		if (bserviced == TRUE) {
			/* schedule EvtInterruptDpc */
			tasklet_schedule(&this_port_device->tasklet_isrdpc);
			retval = IRQ_HANDLED;
			hwdcan_disableinterrupt(
				this_port_device->hwdcan,
				&this_port_device->scaninterrupt);
			hwdcan_enableinterrupt(
				this_port_device->hwdcan,
				&this_port_device->scaninterrupt);
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
	unsigned int start;
	unsigned int end;
	unsigned int count;
	unsigned int bempty;
	struct caninterrupt sinterrupt = {0};
	struct canvolatileinfo svolatileinfo = {0};

	struct net_device *candev;
	struct this_canpriv *canpriv;
	unsigned int txindex;
	struct cantransmitstatus stransmitstatus;
	enum can_state canstate;
	unsigned int i;

	this_port_device = &thismodule_port_devices[moduleportindex];
	pinterrupt_ext = &this_port_device->sinterrupt_extension;
	candev = this_port_device->candev;
	canpriv = this_port_device->canpriv;

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
		/* hwdcan_DCPForISR() */

		if (sinterrupt.receive) {
			/*pr_info("    numrxmessages:%u\n",*/
				/*svolatileinfo.numrxmessages);*/
			for (i = 0; i < svolatileinfo.numrxmessages; i++)
				_candev_rx(candev, &svolatileinfo.rxbuffer[i]);
		}
		if (sinterrupt.txbuffer0transmit) {
			txindex = 0;
			hwdcan_gettransmitstatus(this_port_device->hwdcan,
				txindex, &stransmitstatus);
			if (stransmitstatus.requestaborted) {
				_candev_tx_aborted(candev, txindex);
			} else if (stransmitstatus.transmitcomplete) {
				_candev_tx(candev, txindex,
					this_port_device->txbufferdlc[txindex]);
			} else {
				_candev_tx_error(candev, txindex,
					&stransmitstatus);
			}
			this_port_device->txbufferflag[txindex] = 0;
			netif_wake_queue(candev);
		}
		if (sinterrupt.txbuffer1transmit) {
			txindex = 1;
			hwdcan_gettransmitstatus(this_port_device->hwdcan,
				txindex, &stransmitstatus);
			if (stransmitstatus.requestaborted) {
				_candev_tx_aborted(candev, txindex);
			} else if (stransmitstatus.transmitcomplete) {
				_candev_tx(candev, txindex,
					this_port_device->txbufferdlc[txindex]);
			} else {
				_candev_tx_error(candev, txindex,
					&stransmitstatus);
			}
			this_port_device->txbufferflag[txindex] = 0;
			netif_wake_queue(candev);
		}
		if (sinterrupt.txbuffer2transmit) {
			txindex = 2;
			hwdcan_gettransmitstatus(this_port_device->hwdcan,
				txindex, &stransmitstatus);
			if (stransmitstatus.requestaborted) {
				_candev_tx_aborted(candev, txindex);
			} else if (stransmitstatus.transmitcomplete) {
				_candev_tx(candev, txindex,
					this_port_device->txbufferdlc[txindex]);
			} else {
				_candev_tx_error(candev, txindex,
					&stransmitstatus);
			}
			this_port_device->txbufferflag[txindex] = 0;
			netif_wake_queue(candev);
		}
		if (sinterrupt.errorwarning) {
			_candev_errorwarning(candev,
				svolatileinfo.txerrorcounter,
				svolatileinfo.rxerrorcounter);
		}
		if (sinterrupt.errorpassive) {
			_candev_errorpassive(candev,
				svolatileinfo.txerrorcounter,
				svolatileinfo.rxerrorcounter);
		}
		if (sinterrupt.busoff)
			_candev_busoff(candev);
		if (sinterrupt.arbitrationlost)
			/*_candev_arbitlost*/
			canpriv->can.can_stats.arbitration_lost += 1;
		if (sinterrupt.rxbuserror &&
			sinterrupt.buserrordirection == 1)
			_candev_rx_error(candev, &svolatileinfo.sreceivestatus);
		if (sinterrupt.rxfifooverrun)
			_candev_rx_overrun(candev);

		if (pinterrupt_ext->start !=
			(start + 1) % INTERRUPTQUEUE_MAXCOUNT)
			pr_warn("    Interrupt_DPC sequence miss\n");
	}

	/* finally, we update can_state.*/
	cb_can_get_state(candev, &canstate);
	canpriv->can.state = canstate;
}

static int _candev_tx_aborted(
	struct net_device *candev,
	unsigned int txindex)
{
	int retval; /* return -ERRNO */
	/*int reterr; check if (reterr < 0) */
	/*unsigned int result; check if (result != 0) */

	struct module_port_device	*this_port_device;
	struct this_canpriv		*canpriv;
	struct net_device_stats		*net_stats;
	struct sk_buff		*skb_err;
	struct can_frame	*errframe;

	/*pr_info("%s Entry\n", __func__);*/
	retval = 0;
	skb_err = NULL;
	errframe = NULL;

	canpriv = netdev_priv(candev);
	this_port_device = &thismodule_port_devices[canpriv->moduleportindex];
	net_stats = &candev->stats;

	/* SocketCAN Handle */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0))		
	can_free_echo_skb(candev, txindex, NULL);
#else
	can_free_echo_skb(candev, txindex);
#endif	
	net_stats->tx_dropped += 1;
	net_stats->tx_aborted_errors += 1;

	/* SocketCAN Error Frame */
	skb_err = alloc_can_err_skb(candev, &errframe);
	if (skb_err == NULL)
		goto funcexit;
	/* reference can/error.h */
	/* we take RequestAborted as a tx timeout.*/
	/* error frame is not a real can frame.*/
	errframe->can_id |= CAN_ERR_TX_TIMEOUT;
	netif_receive_skb(skb_err);

	goto funcexit;
funcexit:
	return retval;
}

static int _candev_tx_error(
	struct net_device *candev,
	unsigned int txindex,
	struct cantransmitstatus *ptransmitstatus)
{
	int retval; /* return -ERRNO */
	/*int reterr; check if (reterr < 0) */
	/*unsigned int result; check if (result != 0) */

	struct module_port_device	*this_port_device;
	struct this_canpriv		*canpriv;
	struct net_device_stats		*net_stats;
	struct sk_buff		*skb_err;
	struct can_frame	*errframe;
	/*pr_info("%s Entry\n", __func__);*/
	retval = 0;
	skb_err = NULL;
	errframe = NULL;

	canpriv = netdev_priv(candev);
	this_port_device = &thismodule_port_devices[canpriv->moduleportindex];
	net_stats = &candev->stats;

	/* SocketCAN Handle */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0))		
	can_free_echo_skb(candev, txindex, NULL);
#else
	can_free_echo_skb(candev, txindex);
#endif	
	net_stats->tx_errors += 1;
	if (ptransmitstatus->busoff) {
		/*_can_busoff()*/
	} else {
		if (ptransmitstatus->arbitrationlost)
			;/*_can_arbitlost()*/
		else if (ptransmitstatus->buserror)
			canpriv->can.can_stats.bus_error += 1;
	}

	/* SocketCAN Error Frame */
	skb_err = alloc_can_err_skb(candev, &errframe);
	if (skb_err == NULL)
		goto funcexit;
	if (ptransmitstatus->busoff) {
		/* when transmit a tx buffer at bus off,
		 * we take this error state as timeout.
		 */
		errframe->can_id |= CAN_ERR_TX_TIMEOUT | CAN_ERR_PROT;
		errframe->data[2] |= CAN_ERR_PROT_TX;
	} else {
		if (ptransmitstatus->arbitrationlost) {
			errframe->can_id |= CAN_ERR_LOSTARB | CAN_ERR_PROT;
			errframe->data[0] =
				ptransmitstatus->arbitlostlocation + 1;
			errframe->data[2] |= CAN_ERR_PROT_TX;
		} else if (ptransmitstatus->buserror) {
			errframe->can_id |= CAN_ERR_BUSERROR | CAN_ERR_PROT;
			errframe->data[2] |= CAN_ERR_PROT_TX;
			if (ptransmitstatus->buserrortype == 1)
				errframe->data[2] |= CAN_ERR_PROT_BIT;
			if (ptransmitstatus->buserrortype == 2)
				errframe->data[2] |= CAN_ERR_PROT_STUFF;
			if (ptransmitstatus->buserrortype == 3)
				errframe->data[3] = CAN_ERR_PROT_LOC_CRC_SEQ;
			if (ptransmitstatus->buserrortype == 4)
				errframe->data[2] |= CAN_ERR_PROT_FORM;
			if (ptransmitstatus->buserrortype == 5)
				errframe->can_id |= CAN_ERR_ACK;
			if (ptransmitstatus->buserrorlocation == 0)
				errframe->data[3] = CAN_ERR_PROT_LOC_SOF;
			if (ptransmitstatus->buserrorlocation >= 1 &&
				ptransmitstatus->buserrorlocation <= 3)
				errframe->data[3] = CAN_ERR_PROT_LOC_ID28_21;
			if (ptransmitstatus->buserrorlocation == 4)
				errframe->data[3] = CAN_ERR_PROT_LOC_ID20_18;
			if (ptransmitstatus->buserrorlocation == 5)
				errframe->data[3] = CAN_ERR_PROT_LOC_SRTR;
			if (ptransmitstatus->buserrorlocation == 6)
				errframe->data[3] = CAN_ERR_PROT_LOC_IDE;
			if (ptransmitstatus->buserrorlocation >= 7 &&
				ptransmitstatus->buserrorlocation <= 8)
				errframe->data[3] = CAN_ERR_PROT_LOC_ID17_13;
			if (ptransmitstatus->buserrorlocation >= 9 &&
				ptransmitstatus->buserrorlocation <= 10)
				errframe->data[3] = CAN_ERR_PROT_LOC_ID12_05;
			if (ptransmitstatus->buserrorlocation >= 11 &&
				ptransmitstatus->buserrorlocation <= 12)
				errframe->data[3] = CAN_ERR_PROT_LOC_ID04_00;
			if (ptransmitstatus->buserrorlocation == 13)
				errframe->data[3] = CAN_ERR_PROT_LOC_RTR;
			if (ptransmitstatus->buserrorlocation == 14)
				errframe->data[3] = CAN_ERR_PROT_LOC_RES1;
			if (ptransmitstatus->buserrorlocation == 15)
				errframe->data[3] = CAN_ERR_PROT_LOC_RES0;
			if (ptransmitstatus->buserrorlocation == 16)
				errframe->data[3] = CAN_ERR_PROT_LOC_DLC;
			if (ptransmitstatus->buserrorlocation == 17)
				errframe->data[3] = CAN_ERR_PROT_LOC_DATA;
			if (ptransmitstatus->buserrorlocation == 18)
				errframe->data[3] = CAN_ERR_PROT_LOC_CRC_SEQ;
			if (ptransmitstatus->buserrorlocation == 19)
				errframe->data[3] = CAN_ERR_PROT_LOC_CRC_DEL;
			if (ptransmitstatus->buserrorlocation == 20)
				errframe->data[3] = CAN_ERR_PROT_LOC_ACK;
			if (ptransmitstatus->buserrorlocation == 21)
				errframe->data[3] = CAN_ERR_PROT_LOC_ACK_DEL;
			if (ptransmitstatus->buserrorlocation == 22)
				errframe->data[3] = CAN_ERR_PROT_LOC_EOF;
			if (ptransmitstatus->buserrorlocation == 23)
				errframe->data[3] = CAN_ERR_PROT_LOC_INTERM;
		}
	}
	netif_receive_skb(skb_err);

	goto funcexit;
funcexit:
	return retval;
}

static int _candev_tx(
	struct net_device *candev,
	unsigned int txindex,
	unsigned int dlc)
{
	int retval; /* return -ERRNO */
	/*int reterr; check if (reterr < 0) */
	/*unsigned int result; check if (result != 0) */

	struct net_device_stats		*net_stats;

	retval = 0;

	net_stats = &candev->stats;

	/* SocketCAN Handle */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 12, 0))		
	can_get_echo_skb(candev, txindex, NULL);
#else
	can_get_echo_skb(candev, txindex);
#endif	

	net_stats->tx_bytes += dlc;
	net_stats->tx_packets += 1;

	goto funcexit;
funcexit:
	return retval;
}


static int _candev_rx_error(
	struct net_device *candev,
	struct canreceivestatus *preceivestatus)
{
	int retval; /* return -ERRNO */
	/*int reterr; check if (reterr < 0) */
	/*unsigned int result; check if (result != 0) */

	struct module_port_device	*this_port_device;
	struct this_canpriv		*canpriv;
	struct net_device_stats		*net_stats;
	struct sk_buff		*skb_err;
	struct can_frame	*errframe;
	/*pr_info("%s Entry\n", __func__);*/

	retval = 0;
	skb_err = NULL;
	errframe = NULL;

	canpriv = netdev_priv(candev);
	this_port_device = &thismodule_port_devices[canpriv->moduleportindex];
	net_stats = &candev->stats;

	/* SocketCAN Handle */
	canpriv->can.can_stats.bus_error += 1;

	/* SocketCAN Error Frame */
	skb_err = alloc_can_err_skb(candev, &errframe);
	if (skb_err == NULL)
		goto funcexit;
	if (preceivestatus->buserror) {
		errframe->can_id |= CAN_ERR_BUSERROR | CAN_ERR_PROT;
		if (preceivestatus->buserrortype == 1)
			errframe->data[2] |= CAN_ERR_PROT_BIT;
		if (preceivestatus->buserrortype == 2)
			errframe->data[2] |= CAN_ERR_PROT_STUFF;
		if (preceivestatus->buserrortype == 3)
			errframe->data[3] = CAN_ERR_PROT_LOC_CRC_SEQ;
		if (preceivestatus->buserrortype == 4)
			errframe->data[2] |= CAN_ERR_PROT_FORM;
		if (preceivestatus->buserrortype == 5)
			errframe->can_id |= CAN_ERR_ACK;
		if (preceivestatus->buserrorlocation == 0)
			errframe->data[3] = CAN_ERR_PROT_LOC_SOF;
		if (preceivestatus->buserrorlocation >= 1 &&
			preceivestatus->buserrorlocation <= 3)
			errframe->data[3] = CAN_ERR_PROT_LOC_ID28_21;
		if (preceivestatus->buserrorlocation == 4)
			errframe->data[3] = CAN_ERR_PROT_LOC_ID20_18;
		if (preceivestatus->buserrorlocation == 5)
			errframe->data[3] = CAN_ERR_PROT_LOC_SRTR;
		if (preceivestatus->buserrorlocation == 6)
			errframe->data[3] = CAN_ERR_PROT_LOC_IDE;
		if (preceivestatus->buserrorlocation >= 7 &&
			preceivestatus->buserrorlocation <= 8)
			errframe->data[3] = CAN_ERR_PROT_LOC_ID17_13;
		if (preceivestatus->buserrorlocation >= 9 &&
			preceivestatus->buserrorlocation <= 10)
			errframe->data[3] = CAN_ERR_PROT_LOC_ID12_05;
		if (preceivestatus->buserrorlocation >= 11 &&
			preceivestatus->buserrorlocation <= 12)
			errframe->data[3] = CAN_ERR_PROT_LOC_ID04_00;
		if (preceivestatus->buserrorlocation == 13)
			errframe->data[3] = CAN_ERR_PROT_LOC_RTR;
		if (preceivestatus->buserrorlocation == 14)
			errframe->data[3] = CAN_ERR_PROT_LOC_RES1;
		if (preceivestatus->buserrorlocation == 15)
			errframe->data[3] = CAN_ERR_PROT_LOC_RES0;
		if (preceivestatus->buserrorlocation == 16)
			errframe->data[3] = CAN_ERR_PROT_LOC_DLC;
		if (preceivestatus->buserrorlocation == 17)
			errframe->data[3] = CAN_ERR_PROT_LOC_DATA;
		if (preceivestatus->buserrorlocation == 18)
			errframe->data[3] = CAN_ERR_PROT_LOC_CRC_SEQ;
		if (preceivestatus->buserrorlocation == 19)
			errframe->data[3] = CAN_ERR_PROT_LOC_CRC_DEL;
		if (preceivestatus->buserrorlocation == 20)
			errframe->data[3] = CAN_ERR_PROT_LOC_ACK;
		if (preceivestatus->buserrorlocation == 21)
			errframe->data[3] = CAN_ERR_PROT_LOC_ACK_DEL;
		if (preceivestatus->buserrorlocation == 22)
			errframe->data[3] = CAN_ERR_PROT_LOC_EOF;
		if (preceivestatus->buserrorlocation == 23)
			errframe->data[3] = CAN_ERR_PROT_LOC_INTERM;
	}
	netif_receive_skb(skb_err);

	goto funcexit;
funcexit:
	return retval;
}


static int _candev_rx_overrun(
	struct net_device *candev)
{
	int retval; /* return -ERRNO */
	/*int reterr; check if (reterr < 0) */
	/*unsigned int result; check if (result != 0) */

	struct net_device_stats		*net_stats;
	struct sk_buff		*skb_err;
	struct can_frame	*errframe;
	/*pr_info("%s Entry\n", __func__);*/

	retval = 0;
	skb_err = NULL;
	errframe = NULL;

	net_stats = &candev->stats;

	/* SocketCAN Handle */
	net_stats->rx_over_errors += 1;
	net_stats->rx_errors += 1;

	/* SocketCAN Error Frame */
	skb_err = alloc_can_err_skb(candev, &errframe);
	if (skb_err == NULL)
		goto funcexit;
	errframe->can_id |= CAN_ERR_CRTL;
	errframe->data[1] = CAN_ERR_CRTL_RX_OVERFLOW;
	netif_receive_skb(skb_err);

	goto funcexit;
funcexit:
	return retval;
}

static int _candev_rx(
	struct net_device *candev,
	struct canbuffer *prxbuffer)
{
	int retval; /* return -ERRNO */
	/*int reterr; check if (reterr < 0) */
	/*unsigned int result; check if (result != 0) */

	struct net_device_stats		*net_stats;
	struct sk_buff		*skb_rx;
	struct can_frame	*rxframe;
	/*pr_info("%s Entry\n", __func__);*/

	retval = 0;
	skb_rx = NULL;
	rxframe = NULL;

	net_stats = &candev->stats;

	/* SocketCAN Handle */
	skb_rx = alloc_can_skb(candev, &rxframe);
	if (skb_rx == NULL) {
		net_stats->rx_dropped += 1;
		goto funcexit;
	}
	rxframe->can_id = 0; /* CAN_SFF_FLAG;*/
	rxframe->can_id |= prxbuffer->id & CAN_SFF_MASK;
	if (prxbuffer->extendedframe == 1) {
		rxframe->can_id = CAN_EFF_FLAG;
		rxframe->can_id |= prxbuffer->id & CAN_EFF_MASK;
	}
	if (prxbuffer->rtrbit == 1)
		rxframe->can_id |= CAN_RTR_FLAG;
	rxframe->can_dlc = prxbuffer->datalengthcode;
	*(unsigned int *)&rxframe->data[0] = prxbuffer->byte0_3;
	*(unsigned int *)&rxframe->data[4] = prxbuffer->byte4_7;
	net_stats->rx_bytes += rxframe->can_dlc;
	net_stats->rx_packets += 1;
	netif_receive_skb(skb_rx);

	goto funcexit;
funcexit:
	return retval;
}


static int _candev_errorwarning(
	struct net_device *candev,
	unsigned int txerrorcounter,
	unsigned int rxerrorcounter)
{
	int retval; /* return -ERRNO */
	/*int reterr; check if (reterr < 0) */
	/*unsigned int result; check if (result != 0) */

	struct this_canpriv		*canpriv;
	struct sk_buff		*skb_err;
	struct can_frame	*errframe;
	/*pr_info("%s Entry\n", __func__);*/

	retval = 0;
	skb_err = NULL;
	errframe = NULL;

	canpriv = netdev_priv(candev);

	/* SocketCAN Handle */
	/* use can_change_state to update can.can_stats += 1;*/
	canpriv->can.can_stats.error_warning += 1;
	/* SocketCAN Error Frame*/
	skb_err = alloc_can_err_skb(candev, &errframe);
	if (skb_err == NULL)
		goto funcexit;
	errframe->can_id |= CAN_ERR_CRTL;
	if (txerrorcounter > 96)
		errframe->data[1] |= CAN_ERR_CRTL_TX_WARNING;
	if (rxerrorcounter > 96)
		errframe->data[1] |= CAN_ERR_CRTL_RX_WARNING;
	netif_receive_skb(skb_err);

	goto funcexit;
funcexit:
	return retval;
}

static int _candev_errorpassive(
	struct net_device *candev,
	unsigned int txerrorcounter,
	unsigned int rxerrorcounter)
{
	int retval; /* return -ERRNO */
	/*int reterr; check if (reterr < 0) */
	/*unsigned int result; check if (result != 0) */

	struct this_canpriv		*canpriv;
	struct sk_buff		*skb_err;
	struct can_frame	*errframe;
	/*pr_info("%s Entry\n", __func__);*/

	retval = 0;
	skb_err = NULL;
	errframe = NULL;

	canpriv = netdev_priv(candev);

	/* SocketCAN Handle */
	/* can use can_change_state to update can.can_stats += 1;*/
	canpriv->can.can_stats.error_passive += 1;
	skb_err = alloc_can_err_skb(candev, &errframe);
	if (skb_err == NULL)
		goto funcexit;
	errframe->can_id |= CAN_ERR_CRTL;
	if (txerrorcounter > 127)
		errframe->data[1] |= CAN_ERR_CRTL_TX_PASSIVE;
	if (rxerrorcounter > 127)
		errframe->data[1] |= CAN_ERR_CRTL_RX_PASSIVE;
	netif_receive_skb(skb_err);
	goto funcexit;
funcexit:
	return retval;
}

static int _candev_busoff(
	struct net_device *candev)
{
	int retval; /* return -ERRNO */
	/*int reterr; check if (reterr < 0) */
	/*unsigned int result; check if (result != 0) */

	struct this_canpriv		*canpriv;
	struct sk_buff		*skb_err;
	struct can_frame	*errframe;
	/*pr_info("%s Entry\n", __func__);*/

	retval = 0;
	skb_err = NULL;
	errframe = NULL;

	canpriv = netdev_priv(candev);

	/* SocketCAN Handle */
	/* use can_change_state to update can.can_stats += 1;*/
	canpriv->can.can_stats.bus_off += 1;
	/* SocketCAN Error Frame*/
	skb_err = alloc_can_err_skb(candev, &errframe);
	if (skb_err == NULL)
		goto funcexit;
	errframe->can_id |= CAN_ERR_BUSOFF;
	netif_receive_skb(skb_err);
	/* SocketCAN Handle auto restart
	 * can_bus_off do netif_carrier_off() and restart can if restart_ms
	 */
	can_bus_off(candev);
		/* restart_ms = auto restart = canpriv->can.restart_ms
		 * let SocketCAN Handle restart, because need
		 * netif_carrier_ok(candev);
		 * can_flush_echo_skb(candev);
		 * can_stats.restarts++;
		 * netif_carrier_on(candev);
		 * to recovery Socket
		 */
	goto funcexit;
funcexit:
	return retval;
}

static int candev_start(
	struct net_device *candev)
{
	int retval; /* return -ERRNO */
	/*int reterr; check if (reterr < 0) */
	/*unsigned int result; check if (result != 0) */

	struct module_port_device	*this_port_device;
	struct this_canpriv		*canpriv;

	retval = 0;
	canpriv = netdev_priv(candev);
	this_port_device = &thismodule_port_devices[canpriv->moduleportindex];

	pr_info("%s Entry, deviceid:%u portnumber:%u\n", __func__,
		this_port_device->deviceid, this_port_device->portnumber);
	/* reset CAN & CANIF */
	hwdcan_hardreset(this_port_device->hwdcan);
	/* re-enable interrupt after reset can */
	hwdcan_enableinterrupt(
		this_port_device->hwdcan, &this_port_device->scaninterrupt);
	/* set GlobalControl according to current control mode*/
	this_port_device->scanglobalconf.silent = 0;
	if (canpriv->can.ctrlmode & CAN_CTRLMODE_LISTENONLY)
		this_port_device->scanglobalconf.silent = 1;
	this_port_device->scanglobalconf.selfreception = 0;
	this_port_device->scanglobalconf.transmitwithnoack = 0;
	if (canpriv->can.ctrlmode & CAN_CTRLMODE_LOOPBACK) {
		this_port_device->scanglobalconf.selfreception = 1;
		this_port_device->scanglobalconf.transmitwithnoack = 1;
	}
	this_port_device->scanglobalconf.arbitrationlostretry = 1;
	this_port_device->scanglobalconf.buserrorretry = 1;
	if (canpriv->can.ctrlmode & CAN_CTRLMODE_ONE_SHOT) {
		this_port_device->scanglobalconf.arbitrationlostretry = 0;
		this_port_device->scanglobalconf.buserrorretry = 0;
	}
	
	this_port_device->scanglobalconf.transmitpriority = 2; //round robin
	
	hwdcan_setglobalconfigure(
		this_port_device->hwdcan,
		&this_port_device->scanglobalconf);
	/* set bit timing */
	cb_can_set_bittiming(candev);
	/* start CAN & IF by Pass */
	hwdcan_start(this_port_device->hwdcan);
	/* init tx queue */
	this_port_device->emptytxindex = 0;
	this_port_device->txbufferflag[0] = 0;
	this_port_device->txbufferflag[1] = 0;
	this_port_device->txbufferflag[2] = 0;
	/* set current operational state to CAN_STATE_ERROR_ACTIVE */
	canpriv->can.state = CAN_STATE_ERROR_ACTIVE;
	goto funcexit;
funcexit:
	return retval;
}

static int candev_stop(
	struct net_device *candev)
{
	int retval; /* return -ERRNO */
	/*int reterr; check if (reterr < 0) */
	/*unsigned int result; check if (result != 0) */

	struct module_port_device	*this_port_device;
	struct this_canpriv		*canpriv;

	retval = 0;
	canpriv = netdev_priv(candev);
	this_port_device = &thismodule_port_devices[canpriv->moduleportindex];

	pr_info("%s Entry, deviceid:%u portnumber:%u\n", __func__,
		this_port_device->deviceid, this_port_device->portnumber);
	/* stop CAN */
	hwdcan_stop(this_port_device->hwdcan);

	/* set current operational state to CAN_STATE_STOPPED */
	canpriv->can.state = CAN_STATE_STOPPED;
	goto funcexit;
funcexit:
	return retval;
}

static int cb_can_open(
	struct net_device *candev)
{
	int retval; /* return -ERRNO */
	int reterr; /* check if (reterr < 0) */
	/*unsigned int result; check if (result != 0) */

	struct module_port_device	*this_port_device;
	struct this_canpriv		*canpriv;

	retval = 0;
	canpriv = netdev_priv(candev);
	this_port_device = &thismodule_port_devices[canpriv->moduleportindex];

	pr_info("%s Entry, deviceid:%u portnumber:%u\n", __func__,
		this_port_device->deviceid, this_port_device->portnumber);
	/* open candev */
	reterr = open_candev(candev);
	if (reterr < 0) {
		pr_err("    open_candev failed\n");
		retval = -EFAULT;
		goto funcexit;
	}
	/* start CAN */
	candev_start(candev);
	/* OpenCount */
	this_port_device->opencount = 1;
	/*enable napi */
	napi_enable(&canpriv->napi);
	/* start netif queue */
	netif_start_queue(candev);
	goto funcexit;
funcexit:
	return retval;
}

static  int cb_can_close(
	struct net_device *candev)
{
	int retval; /* return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/*unsigned int result; check if (result != 0) */

	struct module_port_device	*this_port_device;
	struct this_canpriv		*canpriv;

	retval = 0;
	canpriv = netdev_priv(candev);
	this_port_device = &thismodule_port_devices[canpriv->moduleportindex];

	pr_info("%s Entry, deviceid:%u portnumber:%u\n", __func__,
		this_port_device->deviceid, this_port_device->portnumber);
	netif_stop_queue(candev);
	napi_disable(&canpriv->napi);
	this_port_device->opencount = 0;
	candev_stop(candev);
	/* close candev */
	close_candev(candev);
	goto funcexit;
funcexit:
	return retval;
}

static int cb_can_start_xmit(
	struct sk_buff *skb,
	struct net_device *candev)
{
	int retval; /* return -ERRNO */
	/* int reterr; check if (reterr < 0) */
	/*unsigned int result; check if (result != 0) */

	struct module_port_device	*this_port_device;
	struct this_canpriv		*canpriv;
	struct net_device_stats		*net_stats;

	struct can_frame *txframe;
	enum can_state canstate;
	unsigned int txindex;
	struct canbuffer scanbuffer;

	retval = NETDEV_TX_OK;
	canpriv = netdev_priv(candev);
	this_port_device = &thismodule_port_devices[canpriv->moduleportindex];
	net_stats = &candev->stats;
	/*pr_info("%s Entry, deviceid:%u portnumber:%u\n", __func__,*/
		/*this_port_device->deviceid, this_port_device->portnumber);*/

	txframe = (struct can_frame *)skb->data;
	if (can_dropped_invalid_skb(candev, skb)) {
		pr_err("    start_xmit can_dropped_invalid_skb\n");
		retval = NETDEV_TX_OK;
		goto funcexit;
	}
	canstate = canpriv->can.state;
	if (canstate == CAN_STATE_BUS_OFF) {
		/*pr_info("    start_xmit when CAN_STATE_BUS_OFF\n");*/
		kfree_skb(skb);
		net_stats->tx_dropped += 1;
		retval = NETDEV_TX_OK;
		goto funcexit;
	}
	if (canstate == CAN_STATE_STOPPED) {
		/*pr_info("    start_xmit when CAN_STATE_STOPPED\n");*/
		kfree_skb(skb);
		net_stats->tx_dropped += 1;
		retval = NETDEV_TX_OK;
		goto funcexit;
	}
	/* find an empty tx buffer. */
	if (this_port_device->emptytxindex == CAN_NUMBER_TXBUFFER)
		this_port_device->emptytxindex = 0;
	txindex = this_port_device->emptytxindex;
	
	spin_lock(&this_port_device->interruptlock_irq);
	if (this_port_device->txbufferflag[txindex] == 0) {
		this_port_device->emptytxindex += 1;
		spin_unlock(&this_port_device->interruptlock_irq);
	} else {
		/*pr_warn("    start_xmit when tx buffer is full.\n");*/
		netif_stop_queue(candev);
		retval = NETDEV_TX_BUSY;
		spin_unlock(&this_port_device->interruptlock_irq);
		goto funcexit;
	}
	scanbuffer.extendedframe = 0;
	scanbuffer.id = txframe->can_id & CAN_SFF_MASK;
	if (txframe->can_id & CAN_EFF_FLAG) {
		scanbuffer.extendedframe = 1;
		scanbuffer.id = txframe->can_id & CAN_EFF_MASK;
	}
	scanbuffer.rtrbit = 0;
	if (txframe->can_id & CAN_RTR_FLAG)
		scanbuffer.rtrbit = 1;
	scanbuffer.datalengthcode = txframe->can_dlc;
	scanbuffer.byte0_3 = *(unsigned int *)&txframe->data[0];
	scanbuffer.byte4_7 = *(unsigned int *)&txframe->data[4];
	/* put skb to candev core echo socket Buffer before sending message. */
	/* because cb_can_start_xmit is called in tasklet. */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 12, 0))		
	can_put_echo_skb(skb, candev, txindex, 0);
#else
	can_put_echo_skb(skb, candev, txindex);
#endif	
	this_port_device->txbufferflag[txindex] = 1;
	this_port_device->txbufferdlc[txindex] = txframe->can_dlc;
	hwdcan_sendtxbuffer(this_port_device->hwdcan, txindex, &scanbuffer);

	goto funcexit;
funcexit:
	return retval;
}

static int cb_can_set_bittiming(
	struct net_device *candev)
{
	int retval; /* return -ERRNO */
	/*int reterr; check if (reterr < 0) */
	/*unsigned int result; check if (result != 0) */

	struct module_port_device	*this_port_device;
	struct this_canpriv		*canpriv;

	retval = 0;
	canpriv = netdev_priv(candev);
	this_port_device = &thismodule_port_devices[canpriv->moduleportindex];

	pr_info("%s Entry, deviceid:%u portnumber:%u\n", __func__,
		this_port_device->deviceid, this_port_device->portnumber);
	/* setting bit timing according to can_priv.bittiming */
	pr_info("    bittiming.brp = 0x%x, prop_seg=%u, phase_seg1=%u phase_seg2=%u sjw=%u\n",
		canpriv->can.bittiming.brp, canpriv->can.bittiming.prop_seg,
		canpriv->can.bittiming.phase_seg1,
		canpriv->can.bittiming.phase_seg2,
		canpriv->can.bittiming.sjw);
	hwdcan_setclockprescale(this_port_device->hwdcan,
		(canpriv->can.bittiming.brp >> 1) - 1);
	hwdcan_setbustiming(this_port_device->hwdcan,
		canpriv->can.bittiming.prop_seg - 1,
		canpriv->can.bittiming.phase_seg1 - 1,
		canpriv->can.bittiming.phase_seg2 - 1,
		canpriv->can.bittiming.sjw - 1,
		0);
	goto funcexit;
funcexit:
	return retval;
}

static int cb_can_set_mode(
	struct net_device *candev,
	enum can_mode canmode)
{
	int retval; /* return -ERRNO */
	/*int reterr; check if (reterr < 0) */
	/*unsigned int result; check if (result != 0) */

	struct module_port_device	*this_port_device;
	struct this_canpriv		*canpriv;

	retval = 0;
	canpriv = netdev_priv(candev);
	this_port_device = &thismodule_port_devices[canpriv->moduleportindex];

	/*pr_info("%s Entry, deviceid:%u portnumber:%u\n", __func__,*/
		/*this_port_device->deviceid, this_port_device->portnumber);*/
	/*setting CAN power mode*/
	switch (canmode) {
	case CAN_MODE_STOP:
		pr_info("    can power mode: CAN_MODE_STOP\n");
		retval = -EOPNOTSUPP;
		break;
	case CAN_MODE_START:
		pr_info("    can power mode: CAN_MODE_START\n");
		candev_start(candev);
		if (netif_queue_stopped(candev))
			netif_wake_queue(candev);
		break;
	case CAN_MODE_SLEEP:
		pr_info("    can power mode: CAN_MODE_SLEEP\n");
		retval = -EOPNOTSUPP;
		break;
	}
	goto funcexit;
funcexit:
	return retval;
}

static int cb_can_get_state(
	const struct net_device *candev,
	enum can_state *pcanstate)
{
	int retval; /* return -ERRNO */
	/*int reterr; check if (reterr < 0) */
	/*unsigned int result; check if (result != 0) */

	struct module_port_device	*this_port_device;
	struct this_canpriv		*canpriv;
	struct canglobalstatus		sglobalstatus;

	retval = 0;
	canpriv = netdev_priv(candev);
	this_port_device = &thismodule_port_devices[canpriv->moduleportindex];

	/*pr_info("%s Entry, deviceid:%u portnumber:%u\n", __func__,*/
		/*this_port_device->deviceid, this_port_device->portnumber);*/
	if (canpriv->can.state == CAN_STATE_STOPPED) {
		*pcanstate = CAN_STATE_STOPPED;
		goto funcexit;
	}
	hwdcan_getglobalstatus(this_port_device->hwdcan, &sglobalstatus);
	*pcanstate = CAN_STATE_ERROR_ACTIVE;
	if (sglobalstatus.errorwarning)
		*pcanstate = CAN_STATE_ERROR_WARNING;
	if (sglobalstatus.errorpassive)
		*pcanstate = CAN_STATE_ERROR_PASSIVE;
	if (sglobalstatus.busoff)
		*pcanstate = CAN_STATE_BUS_OFF;
goto funcexit;
funcexit:
	return retval;
}

static int cb_can_get_berr_counter(
	const struct net_device *candev,
	struct can_berr_counter *pcanberrcounter)
{
	int retval; /* return -ERRNO */
	/*int reterr; check if (reterr < 0) */
	/*unsigned int result; check if (result != 0) */

	struct module_port_device	*this_port_device;
	struct this_canpriv		*canpriv;
	u32	txerrorcounter;
	u32	rxerrorcounter;

	retval = 0;
	canpriv = netdev_priv(candev);
	this_port_device = &thismodule_port_devices[canpriv->moduleportindex];

	/*pr_info("%s Entry, deviceid:%u portnumber:%u\n", __func__,*/
		/*this_port_device->deviceid, this_port_device->portnumber);*/
	hwdcan_geterrorcounter(
		this_port_device->hwdcan, &txerrorcounter, &rxerrorcounter);
	pcanberrcounter->txerr = txerrorcounter;
	pcanberrcounter->rxerr = rxerrorcounter;
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
	unsigned int	sourceclock;

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
MODULE_DESCRIPTION("RDC SocketCAN Network Device Driver");
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
	platform_17f3_1070 = 0, /* fixed on board */
};

static int platform_default_init(
	struct platform_driver_data *this_driver_data);
static int platform_default_setup(
	struct platform_driver_data *this_driver_data);
static void platform_default_exit(
	struct platform_driver_data *this_driver_data);

static struct platform_specific thismodule_platform_specifics[] = {
	[platform_17f3_1070] = {
		.vendor		= 0x17f3,
		.device		= 0x1070,
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
		.reg_length	= 128,
		.sourceclock	= 20000000,
	},
};

/*
 * ----
 * define pnp device table for this module supports
 * PNP_ID_LEN = 8
 */
static const struct pnp_device_id pnp_device_id_table[] = {
	{ .id = "PNP1070", .driver_data = platform_17f3_1070, },
	{ .id = "AHC0512", .driver_data = platform_17f3_1070, },
	{ } /* terminate list */
};
MODULE_DEVICE_TABLE(pnp, pnp_device_id_table);

static const struct platform_device_id platform_device_id_table[] = {
	/* we take driver_data as a specific number(ID). */
	{ .name = "PNP1070", .driver_data = platform_17f3_1070, },
	{ .name = "AHC0512", .driver_data = platform_17f3_1070, },
	{ } /* terminate list */
};
MODULE_DEVICE_TABLE(platform, platform_device_id_table);

static const struct of_device_id of_device_id_table[] = {
	{ .compatible = "rdc,ahc-can", .data = platform_17f3_1070, },
	{},
};
MODULE_DEVICE_TABLE(of, of_device_id_table);

#ifdef CONFIG_ACPI
static const struct acpi_device_id acpi_device_id_table[] = {
	{ "PNP1070", platform_17f3_1070, },
	{ "AHC0512", platform_17f3_1070, },
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
			this_specific->sourceclock);

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

