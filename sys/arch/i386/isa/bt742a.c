/*
 * Written by Julian Elischer (julian@tfs.com)
 * for TRW Financial Systems for use under the MACH(2.5) operating system.
 *
 * TRW Financial Systems, in accordance with their agreement with Carnegie
 * Mellon University, makes this software available to CMU to distribute
 * or use in any manner that they see fit as long as this message is kept with
 * the software. For this reason TFS also grants any other persons or
 * organisations permission to use or modify this software.
 *
 * TFS supplies this software to be publicly redistributed
 * on the understanding that TFS is not responsible for the correct
 * functioning of this software in any circumstances.
 *
 *	$Id: bt742a.c,v 1.20 1994/03/25 07:40:55 mycroft Exp $
 */

/*
 * BusTech/BusLogic SCSI card driver (all cards)
 *
 * Modified to support round-robin mailbox allocation and page-aligned
 *   buffer allocation by Michael VanLoon (michaelv@iastate.edu)
 */

#include "bt.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <machine/cpu.h>
#include <machine/pio.h>
#include <i386/isa/isa_device.h>

#include <scsi/scsi_all.h>
#include <scsi/scsiconf.h>

#ifdef DDB
int     Debugger();
#else
#define	Debugger() panic("should call debugger here (bt742a.c)")
#endif

extern int delaycount;		/* from clock setup code */
typedef unsigned long int physaddr;

/*
 * I/O Port Interface
 */
#define	BT_BASE			bt_base[unit]
#define	BT_CTRL_STAT_PORT	(BT_BASE + 0x0)	/* control & status */
#define	BT_CMD_DATA_PORT	(BT_BASE + 0x1)	/* cmds and datas */
#define	BT_INTR_PORT		(BT_BASE + 0x2)	/* Intr. stat */

/*
 * BT_CTRL_STAT bits (write)
 */
#define BT_HRST		0x80	/* Hardware reset */
#define BT_SRST		0x40	/* Software reset */
#define BT_IRST		0x20	/* Interrupt reset */
#define BT_SCRST	0x10	/* SCSI bus reset */

/*
 * BT_CTRL_STAT bits (read)
 */
#define BT_STST		0x80	/* Self test in Progress */
#define BT_DIAGF	0x40	/* Diagnostic Failure */
#define BT_INIT		0x20	/* Mbx Init required */
#define BT_IDLE		0x10	/* Host Adapter Idle */
#define BT_CDF		0x08	/* cmd/data out port full */
#define BT_DF		0x04	/* Data in port full */
#define BT_INVDCMD	0x01	/* Invalid command */

/*
 * BT_CMD_DATA bits (write)
 */
#define	BT_NOP			0x00	/* No operation */
#define BT_MBX_INIT		0x01	/* Mbx initialization */
#define BT_START_SCSI		0x02	/* start scsi command */
#define BT_START_BIOS		0x03	/* start bios command */
#define BT_INQUIRE		0x04	/* Adapter Inquiry */
#define BT_MBO_INTR_EN		0x05	/* Enable MBO available interrupt */
#define BT_SEL_TIMEOUT_SET	0x06	/* set selection time-out */
#define BT_BUS_ON_TIME_SET	0x07	/* set bus-on time */
#define BT_BUS_OFF_TIME_SET	0x08	/* set bus-off time */
#define BT_SPEED_SET		0x09	/* set transfer speed */
#define BT_DEV_GET		0x0a	/* return installed devices */
#define BT_CONF_GET		0x0b	/* return configuration data */
#define BT_TARGET_EN		0x0c	/* enable target mode */
#define BT_SETUP_GET		0x0d	/* return setup data */
#define BT_WRITE_CH2		0x1a	/* write channel 2 buffer */
#define BT_READ_CH2		0x1b	/* read channel 2 buffer */
#define BT_WRITE_FIFO		0x1c	/* write fifo buffer */
#define BT_READ_FIFO		0x1d	/* read fifo buffer */
#define BT_ECHO			0x1e	/* Echo command data */
#define BT_MBX_INIT_EXTENDED	0x81	/* Mbx initialization */
#define BT_INQUIRE_EXTENDED	0x8D	/* Adapter Setup Inquiry */

struct bt_cmd_buf {
	u_char  byte[16];
};

/*
 * BT_INTR_PORT bits (read)
 */
#define BT_ANY_INTR		0x80	/* Any interrupt */
#define BT_SCRD		0x08	/* SCSI reset detected */
#define BT_HACC		0x04	/* Command complete */
#define BT_MBOA		0x02	/* MBX out empty */
#define BT_MBIF		0x01	/* MBX in full */

/*
 * Mail box defs
 */
#define BT_MBX_SIZE		32	/* mail box size */

struct bt_mbx {
	struct bt_mbx_out {
		physaddr ccb_addr;
		unsigned char dummy[3];
		unsigned char cmd;
	} mbo[BT_MBX_SIZE];
	struct bt_mbx_in {
		physaddr ccb_addr;
		unsigned char btstat;
		unsigned char sdstat;
		unsigned char dummy;
		unsigned char stat;
	} mbi[BT_MBX_SIZE];
};

/*
 * mbo.cmd values
 */
#define BT_MBO_FREE	0x0	/* MBO entry is free */
#define BT_MBO_START	0x1	/* MBO activate entry */
#define BT_MBO_ABORT	0x2	/* MBO abort entry */

#define BT_MBI_FREE	0x0	/* MBI entry is free */
#define BT_MBI_OK	0x1	/* completed without error */
#define BT_MBI_ABORT	0x2	/* aborted ccb */
#define BT_MBI_UNKNOWN	0x3	/* Tried to abort invalid CCB */
#define BT_MBI_ERROR	0x4	/* Completed with error */

#define	BT_NSEG	32

struct bt_scat_gath {
	unsigned long seg_len;
	physaddr seg_addr;
};

struct bt_ccb {
	unsigned char opcode;
	unsigned char:3, data_in:1, data_out:1,:3;
	unsigned char scsi_cmd_length;
	unsigned char req_sense_length;
	/*------------------------------------longword boundary */
	unsigned long data_length;
	/*------------------------------------longword boundary */
	physaddr data_addr;
	/*------------------------------------longword boundary */
	unsigned char dummy[2];
	unsigned char host_stat;
	unsigned char target_stat;
	/*------------------------------------longword boundary */
	unsigned char target;
	unsigned char lun;
	unsigned char scsi_cmd[12];	/* 12 bytes (bytes only) */
	unsigned char dummy2[1];
	unsigned char link_id;
	/*------------------------------------4 longword boundary */
	physaddr link_addr;
	/*------------------------------------longword boundary */
	physaddr sense_ptr;
	/*------------------------------------longword boundary */
	struct scsi_sense_data scsi_sense;
	/*------------------------------------longword boundary */
	struct bt_scat_gath scat_gath[BT_NSEG];
	/*------------------------------------longword boundary */
	struct bt_ccb *next;
	/*------------------------------------longword boundary */
	struct scsi_xfer *xfer;	/* the scsi_xfer for this cmd */
	/*------------------------------------longword boundary */
	struct bt_mbx_out *mbx;	/* pointer to mail box */
	/*------------------------------------longword boundary */
	long    delta;		/* difference from previous */
	struct bt_ccb *later, *sooner;
	int     flags;
#define	CCB_FREE	0
#define CCB_ACTIVE	1
#define	CCB_ABORTED	2
	unsigned char dummy3[24];	/* align struct to 32 bits */
};

struct bt_ccb *bt_soonest = (struct bt_ccb *) 0;
struct bt_ccb *bt_latest = (struct bt_ccb *) 0;
long int bt_furthest = 0;	/* longest time in the timeout queue */

/*
 * opcode fields
 */
#define BT_INITIATOR_CCB	0x00	/* SCSI Initiator CCB */
#define BT_TARGET_CCB		0x01	/* SCSI Target CCB */
#define BT_INIT_SCAT_GATH_CCB	0x02	/* SCSI Initiator with scattter gather */
#define BT_RESET_CCB		0x81	/* SCSI Bus reset */

/*
 * bt_ccb.host_stat values
 */
#define BT_OK		0x00	/* cmd ok */
#define BT_LINK_OK	0x0a	/* Link cmd ok */
#define BT_LINK_IT	0x0b	/* Link cmd ok + int */
#define BT_SEL_TIMEOUT	0x11	/* Selection time out */
#define BT_OVER_UNDER	0x12	/* Data over/under run */
#define BT_BUS_FREE	0x13	/* Bus dropped at unexpected time */
#define BT_INV_BUS	0x14	/* Invalid bus phase/sequence */
#define BT_BAD_MBO	0x15	/* Incorrect MBO cmd */
#define BT_BAD_CCB	0x16	/* Incorrect ccb opcode */
#define BT_BAD_LINK	0x17	/* Not same values of LUN for links */
#define BT_INV_TARGET	0x18	/* Invalid target direction */
#define BT_CCB_DUP	0x19	/* Duplicate CCB received */
#define BT_INV_CCB	0x1a	/* Invalid CCB or segment list */
#define BT_ABORTED	42	/* pseudo value from driver */

struct bt_setup {
	u_char	sync_neg:1;
	u_char	parity:1;
	u_char	xxx:6;
	u_char	speed;
	u_char	bus_on;
	u_char	bus_off;
	u_char	num_mbx;
	u_char	mbx[4];
	struct {
		u_char	offset:4;
		u_char	period:3;
		u_char	valid:1;
	} sync[8];
	u_char	disc_sts;
};

struct bt_config {
	u_char	chan;
	u_char	intr;
	u_char	scsi_dev:3;
	u_char	xxx:5;
};

#define INT9	0x01
#define INT10	0x02
#define INT11	0x04
#define INT12	0x08
#define INT14	0x20
#define INT15	0x40

#define EISADMA	0x00
#define CHAN0	0x01
#define CHAN5	0x20
#define CHAN6	0x40
#define CHAN7	0x80

#define KVTOPHYS(x)	vtophys(x)

#define PAGESIZ 	NBPG
#define INVALIDATE_CACHE {asm volatile( ".byte	0x0F ;.byte 0x08" ); }

short   bt_base[NBT];		/* base port for each board */
struct isa_device *btinfo[NBT];
struct bt_ccb *bt_get_ccb();
int     bt_int[NBT];
int     bt_dma[NBT];
int     bt_initialized[NBT];

/* we'll malloc memory for these in bt_init() */
struct bt_data {
	struct	bt_mbx bt_mbx;
	struct	bt_ccb bt_ccb[BT_MBX_SIZE];
	struct	scsi_xfer bt_scsi_xfer;
	int	sleepers;
} *btdata[NBT];

struct bt_ccb_lu {
	struct	bt_ccb *kv_addr;
	physaddr phys_addr;
} bt_ccb_lut[NBT][BT_MBX_SIZE];

struct {			/* mbo and mbi last used */
	u_char	mbo;
	u_char	mbi;
} bt_last[NBT];

/***********debug values *************/
#define	BT_SHOWCCBS 0x01
#define	BT_SHOWINTS 0x02
#define	BT_SHOWCMDS 0x04
#define	BT_SHOWMISC 0x08
int     bt_debug = 0;

int     btprobe(), btattach();
int     btintr();

struct isa_driver btdriver = {
	btprobe,
	btattach,
	"bt"
};

static int btunit = 0;

#define bt_abortmbx(mbx) \
	(mbx)->cmd = BT_MBO_ABORT; \
	outb(BT_CMD_DATA_PORT, BT_START_SCSI);
#define bt_startmbx(mbx) \
	(mbx)->cmd = BT_MBO_START; \
	outb(BT_CMD_DATA_PORT, BT_START_SCSI);

int     bt_scsi_cmd();
int     bt_timeout();
void    btminphys();
long int bt_adapter_info();

struct scsi_switch bt_switch[NBT];

#define BT_CMD_TIMEOUT_FUDGE	200	/* multiplied to get Secs */
#define BT_RESET_TIMEOUT	1000000	/* */
#define BT_SCSI_TIMEOUT_FUDGE	20	/* divided by for mSecs */

/*
 * Activate Adapter command
 *	icnt:	number of args (outbound bytes written after opcode)
 *	ocnt:	number of expected returned bytes
 *	wait:   number of seconds to wait for response
 *	retval:	buffer where to place returned bytes
 *	opcode:	opcode BT_NOP, BT_MBX_INIT, BT_START_SCSI ...
 *	args:	parameters
 *
 * Performs an adapter command through the ports. Not to be confused
 *	with a scsi command, which is read in via the dma
 * One of the adapter commands tells it to read in a scsi command
 */
bt_cmd(unit, icnt, ocnt, wait, retval, opcode, args)
	u_char *retval;
	unsigned opcode;
	u_char  args;
{
	unsigned *ic = &opcode;
	u_char  oc;
	register i;
	int     sts;
	struct bt_data *bt = btdata[unit];

	/*
	 * multiply the wait argument by a big constant
	 * zero defaults to 1
	 */
	if (!wait)
		wait = BT_CMD_TIMEOUT_FUDGE * delaycount;
	else
		wait *= BT_CMD_TIMEOUT_FUDGE * delaycount;

	/*
	 * Wait for the adapter to go idle, unless it's one of
	 * the commands which don't need this
	 */
	if (opcode != BT_MBX_INIT && opcode != BT_START_SCSI) {
		i = BT_CMD_TIMEOUT_FUDGE * delaycount;	/* 1 sec? */
		while (--i) {
			sts = inb(BT_CTRL_STAT_PORT);
			if (sts & BT_IDLE) {
				break;
			}
		}
		if (!i) {
			printf("bt_cmd: bt742a host not idle(0x%x)\n", sts);
			return (ENXIO);
		}
	}
	/*
	 * Now that it is idle, if we expect output, preflush the*
	 * queue feeding to us.
	 */
	if (ocnt) {
		while ((inb(BT_CTRL_STAT_PORT)) & BT_DF)
			inb(BT_CMD_DATA_PORT);
	}
	/*
	 * Output the command and the number of arguments given
	 * for each byte, first check the port is empty.
	 */
	icnt++;			/* include the command */
	while (icnt--) {
		sts = inb(BT_CTRL_STAT_PORT);
		for (i = 0; i < wait; i++) {
			sts = inb(BT_CTRL_STAT_PORT);
			if (!(sts & BT_CDF))
				break;
		}
		if (i >= wait) {
			printf("bt_cmd: bt742a cmd/data port full\n");
			outb(BT_CTRL_STAT_PORT, BT_SRST);
			return (ENXIO);
		}
		outb(BT_CMD_DATA_PORT, (u_char) (*ic++));
	}
	/*
	 * If we expect input, loop that many times, each time,
	 * looking for the data register to have valid data
	 */
	while (ocnt--) {
		sts = inb(BT_CTRL_STAT_PORT);
		for (i = 0; i < wait; i++) {
			sts = inb(BT_CTRL_STAT_PORT);
			if (sts & BT_DF)
				break;
		}
		if (i >= wait) {
			printf("bt_cmd: bt742a cmd/data port empty %d\n", ocnt);
			return (ENXIO);
		}
		oc = inb(BT_CMD_DATA_PORT);
		if (retval)
			*retval++ = oc;
	}
	/*
	 * Wait for the board to report a finised instruction
	 */
	i = BT_CMD_TIMEOUT_FUDGE * delaycount;	/* 1 sec? */
	while (--i) {
		sts = inb(BT_INTR_PORT);
		if (sts & BT_HACC) {
			break;
		}
	}
	if (!i) {
		printf("bt_cmd: bt742a host not finished(0x%x)\n", sts);
		return (ENXIO);
	}
	outb(BT_CTRL_STAT_PORT, BT_IRST);
	return (0);
}

/*
 * Check if the device can be found at the port given
 * and if so, set it up ready for further work
 * as an argument, takes the isa_device structure from
 * autoconf.c
 */
btprobe(dev)
	struct isa_device *dev;
{
	/*
	 * find unit and check we have that many defined
	 */
	int     unit;
	struct bt_data *bt;

	if (dev->id_parent)
		return 1;

	dev->id_unit = unit = btunit;
	bt = btdata[unit];
	bt_base[unit] = dev->id_iobase;
	if (unit >= NBT) {
		printf("bt: unit number (%d) too high\n", unit);
		return (0);
	}
	/*
	 * Try initialise a unit at this location
	 * sets up dma and bus speed, loads bt_int[unit]*
	 */
	if (bt_init(unit) != 0) {
		return (0);
	}
	/*
	 * If it's there, put in it's interrupt vectors
	 */
	dev->id_irq = (1 << bt_int[unit]);
	dev->id_drq = bt_dma[unit];

	btunit++;
	return (8);
}

/*
 * Attach all the sub-devices we can find
 */
btattach(dev)
	struct isa_device *dev;
{
	static int firsttime;
	static int firstswitch[NBT];
	int     masunit;
	int     r;

	if (!dev->id_parent)
		return 1;
	masunit = dev->id_parent->id_unit;

	if (!firstswitch[masunit]) {
		firstswitch[masunit] = 1;
		bt_switch[masunit].name = "bt";
		bt_switch[masunit].scsi_cmd = bt_scsi_cmd;
		bt_switch[masunit].scsi_minphys = btminphys;
		bt_switch[masunit].open_target_lu = 0;
		bt_switch[masunit].close_target_lu = 0;
		bt_switch[masunit].adapter_info = bt_adapter_info;
		for (r = 0; r < 8; r++) {
			bt_switch[masunit].empty[r] = 0;
			bt_switch[masunit].used[r] = 0;
			bt_switch[masunit].printed[r] = 0;
		}
	}
	r = scsi_attach(masunit, &bt_switch[masunit], &dev->id_physid,
	    &dev->id_unit, dev->id_flags);

	/* only one for all boards */
	if (firsttime == 0) {
		firsttime = 1;
		bt_timeout(0);
	}
	return r;
}

/*
 * Return some information to the caller about
 * the adapter and it's capabilities
 */
long
bt_adapter_info(unit)
	int     unit;
{
	/* 2 outstanding requests at a time per device */
	return (2);
}


/*
 * Catch an interrupt from the adaptor
 */
btintr(unit)
{
	struct bt_ccb *ccb;
	unsigned char stat;
	register int i, j;
	u_char  found_one = 0, done = 0;
	struct bt_data *bt = btdata[unit];

	if ((scsi_debug & PRINTROUTINES) || bt_debug)
		printf("btintr ");
	/*
	 * First acknowlege the interrupt, Then if it's
	 * not telling about a completed operation
	 * just return.
	 */
	stat = inb(BT_INTR_PORT);
	outb(BT_CTRL_STAT_PORT, BT_IRST);
	if ((scsi_debug & TRACEINTERRUPTS) || bt_debug)
		printf("int = 0x%x ", stat);
	if (!(stat & BT_MBIF))
		return 1;
	if (scsi_debug & TRACEINTERRUPTS)
		printf("mbxi ");

	/*
	 * If it IS then process the competed operation
	 */
	for (i = bt_last[unit].mbi; !done && (i < BT_MBX_SIZE); i++) {
		if (bt->bt_mbx.mbi[i].stat != BT_MBI_FREE) {
			found_one++;
			for (j = BT_MBX_SIZE - 1; j >= 0; j--)
				if (bt_ccb_lut[unit][j].phys_addr ==
				    bt->bt_mbx.mbi[i].ccb_addr) {
					ccb = bt_ccb_lut[unit][j].
					    kv_addr;
					break;
				}
			if ((bt_debug & BT_SHOWCCBS) && ccb)
				printf("<int ccb(%x(%x))> ", ccb, KVTOPHYS(ccb));
			if ((stat = bt->bt_mbx.mbi[i].stat) != BT_MBI_OK) {
				switch (stat) {
				case BT_MBI_ABORT:
					if (bt_debug & BT_SHOWMISC)
						printf("abort ");
					ccb->host_stat = BT_ABORTED;
					break;
				case BT_MBI_UNKNOWN:
					ccb = (struct bt_ccb *) 0;
					if (bt_debug & BT_SHOWMISC)
						printf("unknown ccb for abort");
					break;
				case BT_MBI_ERROR:
					break;
				default:
					printf("bad mbxi status %d, in mbx at 0x%x (0x%x)\n",
						stat, &bt->bt_mbx.mbi[i],
						KVTOPHYS(&bt->bt_mbx.mbi[i]));
					Debugger();
				}
				if ((bt_debug & BT_SHOWCMDS) && ccb) {
					u_char *cp;
					cp = ccb->scsi_cmd;
					printf("op=%x %x %x %x %x %x\n",
					    cp[0], cp[1], cp[2],
					    cp[3], cp[4], cp[5]);
					printf("stat %x for mbi[%d]\n",
					    bt->bt_mbx.mbi[i].stat, i);
					printf("addr = 0x%x\n", ccb);
				}
			}
			if (ccb) {
				bt_remove_timeout(ccb);
				bt_done(unit, ccb);
			}
			bt->bt_mbx.mbi[i].stat = BT_MBI_FREE;
		} else {
			/* free mailbox -- done if following a used mbi */
			if (found_one)
				done++;
		}
	}
	if (done) {
		bt_last[unit].mbi = i % BT_MBX_SIZE;
		return (1);
	}
	for (i = 0; !done && (i < bt_last[unit].mbi); i++) {
		if (bt->bt_mbx.mbi[i].stat != BT_MBI_FREE) {
			found_one++;
			for (j = BT_MBX_SIZE - 1; j >= 0; j--)
				if (bt_ccb_lut[unit][j].phys_addr ==
				    bt->bt_mbx.mbi[i].ccb_addr) {
					ccb = bt_ccb_lut[unit][j].
					    kv_addr;
					break;
				}
			if ((bt_debug & BT_SHOWCCBS) && ccb)
				printf("<int ccb(%x(%x))> ", ccb, KVTOPHYS(ccb));
			if ((stat = bt->bt_mbx.mbi[i].stat) != BT_MBI_OK) {
				switch (stat) {
				case BT_MBI_ABORT:
					if (bt_debug & BT_SHOWMISC)
						printf("abort ");
					ccb->host_stat = BT_ABORTED;
					break;
				case BT_MBI_UNKNOWN:
					ccb = (struct bt_ccb *) 0;
					if (bt_debug & BT_SHOWMISC)
						printf("unknown ccb for abort");
					break;
				case BT_MBI_ERROR:
					break;
				default:
					printf("bad mbxi status %d, in mbx at 0x%x (0x%x)\n",
						stat, &bt->bt_mbx.mbi[i],
						KVTOPHYS(&bt->bt_mbx.mbi[i]));
					Debugger();
				}
				if ((bt_debug & BT_SHOWCMDS) && ccb) {
					u_char *cp;
					cp = ccb->scsi_cmd;
					printf("op=%x %x %x %x %x %x\n",
					    cp[0], cp[1], cp[2],
					    cp[3], cp[4], cp[5]);
					printf("stat %x for mbi[%d]\n",
					    bt->bt_mbx.mbi[i].stat, i);
					printf("addr = 0x%x\n", ccb);
				}
			}
			if (ccb) {
				bt_remove_timeout(ccb);
				bt_done(unit, ccb);
			}
			bt->bt_mbx.mbi[i].stat = BT_MBI_FREE;
		} else {
			/* free mailbox -- done if following a used mbi */
			if (found_one)
				done++;
		}
	}
	bt_last[unit].mbi = i % BT_MBX_SIZE;
	return (1);
}

/*
 * A ccb (and hence a mbx-out is put onto the
 * free list.
 */
bt_free_ccb(unit, ccb, flags)
	struct bt_ccb *ccb;
{
	unsigned int opri;
	struct bt_data *bt = btdata[unit];

	if (scsi_debug & PRINTROUTINES)
		printf("ccb%d(0x%x)> ", unit, flags);
	if (!(flags & SCSI_NOMASK))
		opri = splbio();

	ccb->flags = CCB_FREE;
	/*
	 * If there were none, wake abybody waiting for
	 * one to come free, starting with queued entries
	 */
	if (bt->sleepers) {
		bt->sleepers = 0;
		wakeup((caddr_t) & bt->sleepers);
	}
	if (!(flags & SCSI_NOMASK))
		splx(opri);
}

/*
 * Get a free ccb (and hence mbox-out entry)
 */
struct bt_ccb *
bt_get_ccb(unit, flags)
{
	unsigned int opri;
	struct bt_ccb *rc = NULL;
	struct bt_data *bt = btdata[unit];
	int     next_mbx = bt_last[unit].mbo;

	if (scsi_debug & PRINTROUTINES)
		printf("<ccb%d(0x%x) ", unit, flags);
	if (!(flags & SCSI_NOMASK))
		opri = splbio();
	/*
	 * If we can and have to, sleep waiting for one
	 * to come free
	 */
	while (!(bt->bt_ccb[next_mbx].flags == CCB_FREE) &&
	    !(flags & SCSI_NOSLEEP)) {
		bt->sleepers = 1;
		sleep((caddr_t) & bt->sleepers, PRIBIO);
	}
	if (bt->bt_ccb[next_mbx].flags == CCB_FREE) {
		rc = &bt->bt_ccb[next_mbx];
		bt_last[unit].mbo = (bt_last[unit].mbo + 1) % BT_MBX_SIZE;
		rc->flags = CCB_ACTIVE;
	}
	if (!(flags & SCSI_NOMASK))
		splx(opri);
	return (rc);
}

/*
 * We have a ccb which has been processed by the
 * adaptor, now we look to see how the operation
 * went. Wake up the owner if waiting
 */
bt_done(unit, ccb)
	struct bt_ccb *ccb;
{
	struct scsi_sense_data *s1, *s2;
	struct scsi_xfer *xs = ccb->xfer;
	struct bt_data *bt = btdata[unit];

	if (scsi_debug & (PRINTROUTINES | TRACEINTERRUPTS))
		printf("bt_done ");
	/*
	 * Otherwise, put the results of the operation
	 * into the xfer and call whoever started it
	 */
	if ((ccb->host_stat != BT_OK
		|| ccb->target_stat != SCSI_OK)
	    && (!(xs->flags & SCSI_ERR_OK))) {

		s1 = &(ccb->scsi_sense);
		s2 = &(xs->sense);

		if (ccb->host_stat) {
			switch (ccb->host_stat) {
			case BT_ABORTED:	/* No response */
			case BT_SEL_TIMEOUT:	/* No response */
				if (bt_debug & BT_SHOWMISC) {
					printf("timeout reported back\n");
				}
				xs->error = XS_TIMEOUT;
				break;
			default:	/* Other scsi protocol messes */
				xs->error = XS_DRIVER_STUFFUP;
				if (bt_debug & BT_SHOWMISC) {
					printf("unexpected host_stat: %x\n",
					    ccb->host_stat);
				}
			}

		} else {
			switch (ccb->target_stat) {
			case 0x02:
				*s2 = *s1;
				xs->error = XS_SENSE;
				break;
			case 0x08:
				xs->error = XS_BUSY;
				break;
			default:
				if (bt_debug & BT_SHOWMISC) {
					printf("unexpected target_stat: %x\n",
					    ccb->target_stat);
				}
				xs->error = XS_DRIVER_STUFFUP;
			}
		}
	} else {		/* All went correctly  OR errors expected */
		xs->resid = 0;
	}
	xs->flags |= ITSDONE;
	bt_free_ccb(unit, ccb, xs->flags);
	if (xs->when_done)
		(*(xs->when_done)) (xs->done_arg, xs->done_arg2);
}

/*
 * Start the board, ready for normal operation
 */
bt_init(unit)
	int     unit;
{
	unsigned char ad[4];
	volatile int i, sts;
	struct bt_config conf;
	struct bt_data *bt;

	/*
	 * reset board, If it doesn't respond, assume
	 * that it's not there.. good for the probe
	 */
	outb(BT_CTRL_STAT_PORT, BT_HRST | BT_SRST);

	for (i = 0; i < BT_RESET_TIMEOUT; i++) {
		sts = inb(BT_CTRL_STAT_PORT);
		if (sts == (BT_IDLE | BT_INIT))
			break;
	}
	if (i >= BT_RESET_TIMEOUT) {
		if (bt_debug & BT_SHOWMISC)
			printf("bt_init: No answer from bt742a board\n");
		return (ENXIO);
	}

	/*
	 * Assume we have a board at this stage
	 * setup dma channel from jumpers and save int
	 * level
	 */
	delay(200);

	bt_cmd(unit, 0, sizeof(conf), 0, &conf, BT_CONF_GET);
	switch (conf.chan) {
	case EISADMA:
		bt_dma[unit] = -1;
		break;
	case CHAN0:
		outb(0x0b, 0x0c);
		outb(0x0a, 0x00);
		bt_dma[unit] = 0;
		break;
	case CHAN5:
		outb(0xd6, 0xc1);
		outb(0xd4, 0x01);
		bt_dma[unit] = 5;
		break;
	case CHAN6:
		outb(0xd6, 0xc2);
		outb(0xd4, 0x02);
		bt_dma[unit] = 6;
		break;
	case CHAN7:
		outb(0xd6, 0xc3);
		outb(0xd4, 0x03);
		bt_dma[unit] = 7;
		break;
	default:
		printf("illegal dma setting %x\n", conf.chan);
		return (EIO);
	}
	switch (conf.intr) {
	case INT9:
		bt_int[unit] = 9;
		break;
	case INT10:
		bt_int[unit] = 10;
		break;
	case INT11:
		bt_int[unit] = 11;
		break;
	case INT12:
		bt_int[unit] = 12;
		break;
	case INT14:
		bt_int[unit] = 14;
		break;
	case INT15:
		bt_int[unit] = 15;
		break;
	default:
		printf("illegal int setting\n");
		return (EIO);
	}
	/* who are we on the scsi bus */
	bt_switch[unit].scsi_dev = conf.scsi_dev;

	printf("bt%d: mbx (%d@) %d, ccb %d * %d, xs %d, bt %d bytes\n", unit,
	    BT_MBX_SIZE, sizeof(struct bt_mbx),
	    BT_MBX_SIZE, sizeof(struct bt_ccb),
	    sizeof(struct scsi_xfer), sizeof(struct bt_data));
	bt = malloc(sizeof(struct bt_data), M_DEVBUF, M_NOWAIT);
	if (!bt) {
		printf("bt%d: cannot malloc buffers\n", unit);
		return (0);
	}
	if (bt_debug)
		printf("bt%d: buffer allocated at 0x%x (0x%x)\n",
		    unit, bt, KVTOPHYS(bt));
	bzero(bt, sizeof(struct bt_data));
	btdata[unit] = bt;

	/*
	 * Initialize mail box
	 */
	*((physaddr *) ad) = KVTOPHYS(&bt->bt_mbx);
	if (bt_debug) {
		printf("bt%d: mailbox struct at 0x%x (0x%x)\n",
		    unit, &bt->bt_mbx, *(physaddr *) ad);
		printf("bt%d: ccb struct at 0x%x (0x%x)\n",
		    unit, bt->bt_ccb, KVTOPHYS(bt->bt_ccb));
		printf("bt%d: xs struct at 0x%x (0x%x)\n",
		    unit, &bt->bt_scsi_xfer, KVTOPHYS(&bt->bt_scsi_xfer));
		printf("bt%d: sleepers at 0x%x (0x%x)\n",
		    unit, &bt->sleepers, KVTOPHYS(&bt->sleepers));
	}
	bt_cmd(unit, 5, 0, 0, 0, BT_MBX_INIT_EXTENDED, BT_MBX_SIZE,
	    ad[0], ad[1], ad[2], ad[3]);

	/*
	 * link the ccb's with the mbox-out entries and
	 * into a free-list
	 */
	bt_last[unit].mbo = bt_last[unit].mbi = 0;
	for (i = 0; i < (BT_MBX_SIZE - 1); i++) {
		bt->bt_ccb[i].next = &bt->bt_ccb[i + 1];
		bt->bt_ccb[i].flags = CCB_FREE;
		bt->bt_ccb[i].mbx = &bt->bt_mbx.mbo[i];
		bt->bt_mbx.mbo[i].ccb_addr = KVTOPHYS(&bt->bt_ccb[i]);
		bt_ccb_lut[unit][i].kv_addr = &bt->bt_ccb[i];
		bt_ccb_lut[unit][i].phys_addr =
		    bt->bt_mbx.mbo[i].ccb_addr;
	}
	bt->bt_ccb[i].next = &bt->bt_ccb[0];	/* loop around to first ccb */
	bt->bt_ccb[i].flags = CCB_FREE;
	bt->bt_ccb[i].mbx = &bt->bt_mbx.mbo[i];
	bt->bt_mbx.mbo[i].ccb_addr = KVTOPHYS(&bt->bt_ccb[i]);
	bt_ccb_lut[unit][i].kv_addr = &bt->bt_ccb[i];
	bt_ccb_lut[unit][i].phys_addr =
	    bt->bt_mbx.mbo[i].ccb_addr;

	/*
	 * Note that we are going and return (to probe)
	 */
	bt_initialized[unit]++;
	return (0);
}


#ifndef	min
#define min(x,y) (x < y ? x : y)
#endif	/* min */

void
btminphys(bp)
	struct buf *bp;
{
	if (bp->b_bcount > ((BT_NSEG - 1) * PAGESIZ)) {
		bp->b_bcount = ((BT_NSEG - 1) * PAGESIZ);
	}
}

/*
 * start a scsi operation given the command and the
 * data address. Also needs the unit, target and lu
 */
int
bt_scsi_cmd(xs)
	struct scsi_xfer *xs;
{
	struct scsi_sense_data *s1, *s2;
	struct bt_ccb *ccb;
	struct bt_scat_gath *sg;
	int     seg;		/* scatter gather seg being worked on */
	int     i = 0;
	int     rc = 0;
	int     thiskv;
	physaddr thisphys, nextphys;
	int     unit = xs->adapter;
	int     bytes_this_seg, bytes_this_page, datalen, flags;
	struct iovec *iovp;
	struct bt_data *bt = btdata[unit];
	int     done, count;

	if (scsi_debug & PRINTROUTINES)
		printf("bt_scsi_cmd ");

	/*
	 * get a ccb (mbox-out) to use. If the transfer
	 * is from a buf (possibly from interrupt time)
	 * then we can't allow it to sleep
	 */
	flags = xs->flags;
	if (xs->bp)
		flags |= (SCSI_NOSLEEP);	/* just to be sure */
	if (flags & ITSDONE) {
		printf("Already done?");
		xs->flags &= ~ITSDONE;
	}
	if (!(flags & INUSE)) {
		printf("Not in use?");
		xs->flags |= INUSE;
	}
	if (!(ccb = bt_get_ccb(unit, flags))) {
		xs->error = XS_DRIVER_STUFFUP;
		return (TRY_AGAIN_LATER);
	}
	if (bt_debug & BT_SHOWCCBS)
		printf("<start ccb(%x(%x))>", ccb, KVTOPHYS(ccb));
	if (ccb->mbx->cmd != BT_MBO_FREE)
		printf("MBO not free (%x(%x))\n",
		    ccb->mbx, KVTOPHYS(ccb->mbx));

	/*
	 * Put all the arguments for the xfer in the ccb
	 */
	ccb->xfer = xs;
	if (flags & SCSI_RESET) {
		ccb->opcode = BT_RESET_CCB;
	} else {
		/* can't use S/G if zero length */
		ccb->opcode = (xs->datalen ? BT_INIT_SCAT_GATH_CCB :
		    BT_INITIATOR_CCB);
	}
	ccb->target = xs->targ;;
	ccb->data_out = 0;
	ccb->data_in = 0;
	ccb->lun = xs->lu;
	ccb->scsi_cmd_length = xs->cmdlen;
	ccb->sense_ptr = KVTOPHYS(&(ccb->scsi_sense));
	ccb->req_sense_length = sizeof(ccb->scsi_sense);

	if ((xs->datalen) && (!(flags & SCSI_RESET))) {
		/* can use S/G only if not zero length */
		ccb->data_addr = KVTOPHYS(ccb->scat_gath);
		sg = ccb->scat_gath;
		seg = 0;
		if (flags & SCSI_DATA_UIO) {
			iovp = ((struct uio *) xs->data)->uio_iov;
			datalen = ((struct uio *) xs->data)->uio_iovcnt;
			xs->datalen = 0;
			while (datalen && (seg < BT_NSEG)) {
				sg->seg_addr = (physaddr) iovp->iov_base;
				xs->datalen += sg->seg_len = iovp->iov_len;
				if (scsi_debug & SHOWSCATGATH)
					printf("(0x%x@0x%x)",
					    iovp->iov_len,
					    iovp->iov_base);
				sg++;
				iovp++;
				seg++;
				datalen--;
			}
		} else {
			/*
			 * Set up the scatter gather block
			 */
			if (scsi_debug & SHOWSCATGATH)
				printf("%d @0x%x:- ", xs->datalen, xs->data);
			datalen = xs->datalen;
			thiskv = (int) xs->data;
			thisphys = KVTOPHYS(thiskv);

			while ((datalen) && (seg < BT_NSEG)) {
				bytes_this_seg = 0;

				/* put in the base address */
				sg->seg_addr = thisphys;

				if (scsi_debug & SHOWSCATGATH)
					printf("0x%x", thisphys);

				/* do it at least once */
				nextphys = thisphys;
				while ((datalen) && (thisphys == nextphys)) {
					/*
					 * This page is contiguous (physically) with
					 * the the last, just extend the length
					 * how far to the end of the page
					 */
					nextphys = (thisphys & (~(PAGESIZ - 1)))
					    + PAGESIZ;
					bytes_this_page = nextphys - thisphys;
					/**** or the data ****/
					bytes_this_page = min(bytes_this_page,
					    datalen);
					bytes_this_seg += bytes_this_page;
					datalen -= bytes_this_page;

					/* get more ready for the next page */
					thiskv = (thiskv & (~(PAGESIZ - 1)))
					    + PAGESIZ;
					if (datalen)
						thisphys = KVTOPHYS(thiskv);
				}

				/*
				 * next page isn't contiguous, finish the seg
				 */
				if (scsi_debug & SHOWSCATGATH)
					printf("(0x%x)", bytes_this_seg);
				sg->seg_len = bytes_this_seg;
				sg++;
				seg++;
			}
		}		/* end of iov/kv decision */
		ccb->data_length = seg * sizeof(struct bt_scat_gath);
		if (scsi_debug & SHOWSCATGATH)
			printf("\n");
		if (datalen) {
			/* there's still data, must have run out of segs! */
			printf("bt_scsi_cmd%d: more than %d DMA segs\n",
			    unit, BT_NSEG);
			xs->error = XS_DRIVER_STUFFUP;
			bt_free_ccb(unit, ccb, flags);
			return (HAD_ERROR);
		}
	} else {
		/* No data xfer, use non S/G values */
		ccb->data_addr = (physaddr) 0;
		ccb->data_length = 0;
	}
	ccb->link_id = 0;
	ccb->link_addr = (physaddr) 0;

	/*
	 * Put the scsi command in the ccb and start it
	 */
	if (!(flags & SCSI_RESET)) {
		bcopy(xs->cmd, ccb->scsi_cmd, ccb->scsi_cmd_length);
	}
	if (scsi_debug & SHOWCOMMANDS) {
		u_char *b = ccb->scsi_cmd;
		if (!(flags & SCSI_RESET)) {
			int     i = 0;
			printf("bt%d:%d:%d-", unit, ccb->target, ccb->lun);
			while (i < ccb->scsi_cmd_length) {
				if (i)
					printf(",");
				printf("%x", b[i++]);
			}
			printf("-\n");
		} else
			printf("bt%d:%d:%d-RESET- ", unit, ccb->target, ccb->lun);
	}
	bt_startmbx(ccb->mbx);

	/*
	 * Usually return SUCCESSFULLY QUEUED
	 */
	if (scsi_debug & TRACEINTERRUPTS)
		printf("cmd_sent ");
	if (!(flags & SCSI_NOMASK)) {
		bt_add_timeout(ccb, xs->timeout);
		return (SUCCESSFULLY_QUEUED);
	}

	/*
	 * If we can't use interrupts, poll on completion
	 */
	done = 0;
	count = delaycount * xs->timeout / BT_SCSI_TIMEOUT_FUDGE;

	if (scsi_debug & TRACEINTERRUPTS)
		printf("wait ");
	while ((!done) && count) {
		i = 0;
		while (!done && i < BT_MBX_SIZE) {
			struct bt_ccb *mbx_ccb = NULL;
			int     j;

			for (j = BT_MBX_SIZE - 1; j >= 0; j--)
				if (bt_ccb_lut[unit][j].phys_addr ==
				    bt->bt_mbx.mbi[i].ccb_addr) {
					mbx_ccb = bt_ccb_lut[unit][j].kv_addr;
					break;
				}
			if ((bt->bt_mbx.mbi[i].stat != BT_MBI_FREE) &&
			    mbx_ccb == ccb) {
				bt->bt_mbx.mbi[i].stat = BT_MBI_FREE;
				bt_done(unit, ccb);
				done++;
			}
			i++;
		}
		count--;
	}
	if (!count) {
		if (!(xs->flags & SCSI_SILENT))
			printf("cmd fail\n");
		bt_abortmbx(ccb->mbx);
		count = delaycount * 2000 / BT_SCSI_TIMEOUT_FUDGE;
		while (!done && count) {
			i = 0;
			while (!done && i < BT_MBX_SIZE) {
				struct bt_ccb *mbx_ccb = NULL;
				int     j;

				for (j = BT_MBX_SIZE - 1; j >= 0; j--)
					if (bt_ccb_lut[unit][j].phys_addr ==
					    bt->bt_mbx.mbi[i].ccb_addr) {
						mbx_ccb = bt_ccb_lut[unit][j].kv_addr;
						break;
					}
				if ((bt->bt_mbx.mbi[i].stat !=
					BT_MBI_FREE) && mbx_ccb == ccb) {
					bt->bt_mbx.mbi[i].stat =
					    BT_MBI_FREE;
					bt_done(unit, ccb);
					done++;
				}
				i++;
			}
			count--;
		}
		if (!count) {
			printf("abort failed in wait\n");
			ccb->mbx->cmd = BT_MBO_FREE;
		}
		bt_free_ccb(unit, ccb, flags);
		btintr(unit);
		xs->error = XS_DRIVER_STUFFUP;
		return (HAD_ERROR);
	}
	btintr(unit);
	if (xs->error)
		return (HAD_ERROR);
	return (COMPLETE);
}

/*
 *               +----------+     +----------+     +----------+
 * bt_soonest--->|    later |---->|     later|---->|     later|--->0
 *               | [Delta]  |     | [Delta]  |     | [Delta]  |
 *        0<-----|sooner    |<----|sooner    |<----|sooner    |<----bt_latest
 *               +----------+     +----------+     +----------+
 *
 *     bt_furthest = sum(Delta[1..n])
 */
bt_add_timeout(ccb, time)
	struct bt_ccb *ccb;
	int     time;
{
	int timeprev;
	struct bt_ccb *prev;
	int s = splbio();

	prev = bt_latest;
	if (prev)
		timeprev = bt_furthest;
	else
		timeprev = 0;

	while (prev && (timeprev > time)) {
		timeprev -= prev->delta;
		prev = prev->sooner;
	}
	if (prev) {
		ccb->delta = time - timeprev;
		if (ccb->later = prev->later) {	/* yes an assign */
			ccb->later->sooner = ccb;
			ccb->later->delta -= ccb->delta;
		} else {
			bt_furthest = time;
			bt_latest = ccb;
		}
		ccb->sooner = prev;
		prev->later = ccb;
	} else {
		if (ccb->later = bt_soonest) {	/* yes, an assign */
			ccb->later->sooner = ccb;
			ccb->later->delta -= time;
		} else {
			bt_furthest = time;
			bt_latest = ccb;
		}
		ccb->delta = time;
		ccb->sooner = (struct bt_ccb *) 0;
		bt_soonest = ccb;
	}
	splx(s);
}

bt_remove_timeout(ccb)
	struct bt_ccb *ccb;
{
	int s = splbio();

	if (ccb->sooner)
		ccb->sooner->later = ccb->later;
	else
		bt_soonest = ccb->later;

	if (ccb->later) {
		ccb->later->sooner = ccb->sooner;
		ccb->later->delta += ccb->delta;
	} else {
		bt_latest = ccb->sooner;
		bt_furthest -= ccb->delta;
	}
	ccb->sooner = ccb->later = (struct bt_ccb *) 0;
	splx(s);
}


extern int hz;
/* #define ONETICK 500 /* milliseconds */
#define ONETICK 250		/* milliseconds */
#define SLEEPTIME ((hz * 1000) / ONETICK)

bt_timeout(arg)
	int     arg;
{
	struct bt_ccb *ccb;
	int     unit;
	int     s = splbio();

	while (ccb = bt_soonest) {
		if (ccb->delta <= ONETICK) {
			/*
			 * It has timed out, we need to do some work
			 */
			unit = ccb->xfer->adapter;
			btintr(unit);
			printf("bt%d:%d device timed out\n", unit,
			    ccb->xfer->targ);
			if (bt_debug & BT_SHOWCCBS)
				tfs_print_active_ccbs();

			/*
			 * Unlink it from the queue
			 */
			bt_remove_timeout(ccb);

			/*
			 * If The ccb's mbx is not free, then
			 * the board has gone south
			 */
			if (ccb->mbx->cmd != BT_MBO_FREE) {
				printf("bt%d not taking commands!\n", unit);
				printf("bt: ccb->mbx->cmd = %x\n",
				    ccb->mbx->cmd);
				tfs_print_ccb(ccb);
				Debugger();
			}
			/*
			 * If it has been through before, then
			 * a previous abort has failed, don't
			 * try abort again
			 */
			if (ccb->flags == CCB_ABORTED) {	/* abort timed out */
				printf("AGAIN");
				ccb->xfer->retries = 0;	/* I MEAN IT ! */
				ccb->host_stat = BT_ABORTED;
				bt_done(unit, ccb);
			} else {/* abort the operation that has timed out */
				printf("abort mbx\n");
				bt_abortmbx(ccb->mbx);
				/* 2 secs for the abort */
				bt_add_timeout(ccb, 2000 + ONETICK);
				ccb->flags = CCB_ABORTED;
			}
		} else {
			/*
			 * It has not timed out, adjust and leave
			 */
			ccb->delta -= ONETICK;
			bt_furthest -= ONETICK;
			break;
		}
	}
	splx(s);
	timeout((timeout_t) bt_timeout, (caddr_t) arg, SLEEPTIME);
}

tfs_print_ccb(ccb)
	struct bt_ccb *ccb;
{
	printf("ccb:%x op:%x cmdlen:%d senlen:%d\n", ccb, ccb->opcode,
	    ccb->scsi_cmd_length, ccb->req_sense_length);
	printf("        datlen:%d hstat:%x tstat:%x delta:%d flags:%x\n",
	    ccb->data_length, ccb->host_stat, ccb->target_stat,
	    ccb->delta, ccb->flags);
}

tfs_print_active_ccbs()
{
	struct bt_ccb *ccb = bt_soonest;

	while (ccb) {
		tfs_print_ccb(ccb);
		ccb = ccb->later;
	}
	printf("Furthest = %d\n", bt_furthest);
}
