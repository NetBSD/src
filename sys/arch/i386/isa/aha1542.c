/*
 * Copyright (c) 1994 Charles Hannum.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Charles Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	$Id: aha1542.c,v 1.31.2.2 1994/07/28 05:24:17 cgd Exp $
 */

/*
 * Originally written by Julian Elischer (julian@tfs.com)
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
 */

/*
 * Ported to run under 386BSD by Julian Elischer (julian@tfs.com) Sept 1992
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/user.h>

#include <machine/pio.h>

#include <i386/isa/isavar.h>
#include <i386/isa/icu.h>
#include <scsi/scsi_all.h>
#include <scsi/scsiconf.h>

#ifdef DDB
int	Debugger();
#else	/* NDDB */
#define Debugger() panic("should call debugger here (adaptec.c)")
#endif	/* NDDB */

/************************** board definitions *******************************/

/*
 * I/O Port Interface
 */
#define	AHA_BASE		aha->aha_base
#define	AHA_CTRL_STAT_PORT	(AHA_BASE + 0x0)	/* control & status */
#define	AHA_CMD_DATA_PORT	(AHA_BASE + 0x1)	/* cmds and datas */
#define	AHA_INTR_PORT		(AHA_BASE + 0x2)	/* Intr. stat */

/*
 * AHA_CTRL_STAT bits (write)
 */
#define AHA_HRST		0x80	/* Hardware reset */
#define AHA_SRST		0x40	/* Software reset */
#define AHA_IRST		0x20	/* Interrupt reset */
#define AHA_SCRST		0x10	/* SCSI bus reset */

/*
 * AHA_CTRL_STAT bits (read)
 */
#define AHA_STST		0x80	/* Self test in Progress */
#define AHA_DIAGF		0x40	/* Diagnostic Failure */
#define AHA_INIT		0x20	/* Mbx Init required */
#define AHA_IDLE		0x10	/* Host Adapter Idle */
#define AHA_CDF			0x08	/* cmd/data out port full */
#define AHA_DF			0x04	/* Data in port full */
#define AHA_INVDCMD		0x01	/* Invalid command */

/*
 * AHA_CMD_DATA bits (write)
 */
#define	AHA_NOP			0x00	/* No operation */
#define AHA_MBX_INIT		0x01	/* Mbx initialization */
#define AHA_START_SCSI		0x02	/* start scsi command */
#define AHA_START_BIOS		0x03	/* start bios command */
#define AHA_INQUIRE		0x04	/* Adapter Inquiry */
#define AHA_MBO_INTR_EN		0x05	/* Enable MBO available interrupt */
#define AHA_SEL_TIMEOUT_SET	0x06	/* set selection time-out */
#define AHA_BUS_ON_TIME_SET	0x07	/* set bus-on time */
#define AHA_BUS_OFF_TIME_SET	0x08	/* set bus-off time */
#define AHA_SPEED_SET		0x09	/* set transfer speed */
#define AHA_DEV_GET		0x0a	/* return installed devices */
#define AHA_CONF_GET		0x0b	/* return configuration data */
#define AHA_TARGET_EN		0x0c	/* enable target mode */
#define AHA_SETUP_GET		0x0d	/* return setup data */
#define AHA_WRITE_CH2		0x1a	/* write channel 2 buffer */
#define AHA_READ_CH2		0x1b	/* read channel 2 buffer */
#define AHA_WRITE_FIFO		0x1c	/* write fifo buffer */
#define AHA_READ_FIFO		0x1d	/* read fifo buffer */
#define AHA_ECHO		0x1e	/* Echo command data */
#define AHA_EXT_BIOS		0x28	/* return extended bios info */
#define AHA_MBX_ENABLE		0x29	/* enable mail box interface */

struct aha_cmd_buf {
	u_char  byte[16];
};

/*
 * AHA_INTR_PORT bits (read)
 */
#define AHA_ANY_INTR		0x80	/* Any interrupt */
#define AHA_SCRD		0x08	/* SCSI reset detected */
#define AHA_HACC		0x04	/* Command complete */
#define AHA_MBOA		0x02	/* MBX out empty */
#define AHA_MBIF		0x01	/* MBX in full */

/*
 * Mail box defs 
 */
#define AHA_MBX_SIZE		16	/* mail box size */

struct aha_mbx {
	struct aha_mbx_out {
		u_char cmd;
		u_char ccb_addr[3];
	} mbo[AHA_MBX_SIZE];
	struct aha_mbx_in {
		u_char stat;
		u_char ccb_addr[3];
	} mbi[AHA_MBX_SIZE];
};

/*
 * mbo.cmd values
 */
#define AHA_MBO_FREE	0x0	/* MBO entry is free */
#define AHA_MBO_START	0x1	/* MBO activate entry */
#define AHA_MBO_ABORT	0x2	/* MBO abort entry */

/*
 * mbi.stat values
 */
#define AHA_MBI_FREE	0x0	/* MBI entry is free */
#define AHA_MBI_OK	0x1	/* completed without error */
#define AHA_MBI_ABORT	0x2	/* aborted ccb */
#define AHA_MBI_UNKNOWN	0x3	/* Tried to abort invalid CCB */
#define AHA_MBI_ERROR	0x4	/* Completed with error */

/* FOR OLD VERSIONS OF THE !%$@ this may have to be 16 (yuk) */
#define	AHA_NSEG	17	/* Number of scatter gather segments <= 16 */
				/* allow 64 K i/o (min) */

struct aha_ccb {
	u_char opcode;
	u_char lun:3;
	u_char data_in:1;	/* must be 0 */
	u_char data_out:1;	/* must be 0 */
	u_char target:3;
	u_char scsi_cmd_length;
	u_char req_sense_length;
	u_char data_length[3];
	u_char data_addr[3];
	u_char link_addr[3];
	u_char link_id;
	u_char host_stat;
	u_char target_stat;
	u_char reserved[2];
	struct scsi_generic scsi_cmd;
	struct scsi_sense_data scsi_sense;
	struct aha_scat_gath {
		u_char seg_len[3];
		u_char seg_addr[3];
	} scat_gath[AHA_NSEG];
	struct aha_ccb *next;
	struct scsi_xfer *xfer;	/* the scsi_xfer for this cmd */
	struct aha_mbx_out *mbx;	/* pointer to mail box */
	int flags;
#define CCB_FREE	0
#define CCB_ACTIVE	1
#define CCB_ABORTED	2
};

/*
 * opcode fields
 */
#define AHA_INITIATOR_CCB	0x00	/* SCSI Initiator CCB */
#define AHA_TARGET_CCB		0x01	/* SCSI Target CCB */
#define AHA_INIT_SCAT_GATH_CCB	0x02	/* SCSI Initiator with scatter gather */
#define AHA_RESET_CCB		0x81	/* SCSI Bus reset */

/*
 * aha_ccb.host_stat values
 */
#define AHA_OK		0x00	/* cmd ok */
#define AHA_LINK_OK	0x0a	/* Link cmd ok */
#define AHA_LINK_IT	0x0b	/* Link cmd ok + int */
#define AHA_SEL_TIMEOUT	0x11	/* Selection time out */
#define AHA_OVER_UNDER	0x12	/* Data over/under run */
#define AHA_BUS_FREE	0x13	/* Bus dropped at unexpected time */
#define AHA_INV_BUS	0x14	/* Invalid bus phase/sequence */
#define AHA_BAD_MBO	0x15	/* Incorrect MBO cmd */
#define AHA_BAD_CCB	0x16	/* Incorrect ccb opcode */
#define AHA_BAD_LINK	0x17	/* Not same values of LUN for links */
#define AHA_INV_TARGET	0x18	/* Invalid target direction */
#define AHA_CCB_DUP	0x19	/* Duplicate CCB received */
#define AHA_INV_CCB	0x1a	/* Invalid CCB or segment list */
#define AHA_ABORTED	42

struct aha_setup {
	u_char  sync_neg:1;
	u_char  parity:1;
		u_char:6;
	u_char  speed;
	u_char  bus_on;
	u_char  bus_off;
	u_char  num_mbx;
	u_char  mbx[3];
	struct {
		u_char  offset:4;
		u_char  period:3;
		u_char  valid:1;
	} sync[8];
	u_char  disc_sts;
};

struct aha_config {
	u_char  chan;
	u_char  intr;
	u_char  scsi_dev:3;
		u_char:5;
};

struct aha_inquire {
	u_char	boardid;		/* type of board */
					/* 0x20 = BusLogic 545, but it gets
					   the command wrong, only returns
					   one byte */
					/* 0x31 = AHA-1540 */
					/* 0x41 = AHA-1540A/1542A/1542B */
					/* 0x42 = AHA-1640 */
					/* 0x43 = AHA-1542C */
					/* 0x44 = AHA-1542CF */
					/* 0x45 = AHA-1542CF, BIOS v2.01 */
	u_char	spec_opts;		/* special options ID */
					/* 0x41 = Board is standard model */
	u_char	revision_1;		/* firmware revision [0-9A-Z] */
	u_char	revision_2;		/* firmware revision [0-9A-Z] */
};

struct aha_extbios {
	u_char	flags;			/* Bit 3 == 1 extended bios enabled */
	u_char	mailboxlock;		/* mail box lock code to unlock it */
};

#define INT9	0x01
#define INT10	0x02
#define INT11	0x04
#define INT12	0x08
#define INT14	0x20
#define INT15	0x40

#define CHAN0	0x01
#define CHAN5	0x20
#define CHAN6	0x40
#define CHAN7	0x80

/*********************************** end of board definitions***************/

#define PHYSTOKV(x)	(((long int)(x)) ^ aha->kv_phys_xor)
#define KVTOPHYS(x)	vtophys(x)

u_char  aha_scratch_buf[256];
#ifdef	AHADEBUG
int	aha_debug = 1;
#endif /*AHADEBUG */

struct aha_softc {
	struct device sc_dev;
	struct isadev sc_id;
	struct intrhand sc_ih;

	u_short aha_base;			/* base port for each board */
	/*
	 * xor this with a physaddr to get a kv addr and visa versa
	 * for items in THIS STRUCT only. 
	 * Used to get the CCD's physical and kv addresses from each
	 * other.
	 */
	long int kv_phys_xor;
	struct aha_mbx aha_mbx;			/* all the mailboxes */
	struct aha_ccb *aha_ccb_free;		/* the next free CCB */
	struct aha_ccb aha_ccb[AHA_MBX_SIZE];	/* all the CCBs */
	u_short aha_int;			/* our IRQ level */
	u_short aha_dma;			/* our DMA req channel */
	int aha_scsi_dev;			/* our SCSI bus address */
	struct scsi_link sc_link;		/* prototype for subdevs */
};

int aha_cmd();	/* XXX must be varargs to prototype */
u_int aha_adapter_info __P((struct aha_softc *));
int ahaintr __P((struct aha_softc *));
void aha_free_ccb __P((struct aha_softc *, struct aha_ccb *, int));
struct aha_ccb *aha_get_ccb __P((struct aha_softc *, int));
void aha_done __P((struct aha_softc *, struct aha_ccb *));
int aha_find __P((struct aha_softc *));
void aha_init __P((struct aha_softc *));
void ahaminphys __P((struct buf *));
int aha_scsi_cmd __P((struct scsi_xfer *));
int aha_poll __P((struct aha_softc *, struct scsi_xfer *, struct aha_ccb *));
int aha_set_bus_speed __P((struct aha_softc *));
int aha_bus_speed_check __P((struct aha_softc *, int));
void aha_timeout __P((void *arg));

struct scsi_adapter aha_switch = {
	aha_scsi_cmd,
	ahaminphys,
	0,
	0,
	aha_adapter_info,
	"aha"
};

/* the below structure is so we have a default dev struct for out link struct */
struct scsi_device aha_dev = {
	NULL,			/* Use default error handler */
	NULL,			/* have a queue, served by this */
	NULL,			/* have no async handler */
	NULL,			/* Use default 'done' routine */
	"aha",
	0
};

int ahaprobe();
void ahaattach();

struct cfdriver ahacd = {
	NULL, "aha", ahaprobe, ahaattach, DV_DULL, sizeof(struct aha_softc)
};

void aha_init __P((struct aha_softc *));

#define aha_abortmbx(mbx) \
	(mbx)->cmd = AHA_MBO_ABORT; \
	outb(AHA_CMD_DATA_PORT, AHA_START_SCSI);
#define aha_startmbx(mbx) \
	(mbx)->cmd = AHA_MBO_START; \
	outb(AHA_CMD_DATA_PORT, AHA_START_SCSI);

#define AHA_RESET_TIMEOUT	2000	/* time to wait for reset (mSec) */

/*
 * aha_cmd(aha,icnt, ocnt,wait, retval, opcode, args)
 * Activate Adapter command
 *    icnt:   number of args (outbound bytes written after opcode)
 *    ocnt:   number of expected returned bytes
 *    wait:   number of seconds to wait for response
 *    retval: buffer where to place returned bytes
 *    opcode: opcode AHA_NOP, AHA_MBX_INIT, AHA_START_SCSI ...
 *    args:   parameters
 *
 * Performs an adapter command through the ports. Not to be confused
 * with a scsi command, which is read in via the dma.  One of the adapter
 * commands tells it to read in a scsi command but that one is done
 * separately.  This is only called during set-up.
 */
int
aha_cmd(aha, icnt, ocnt, wait, retval, opcode, args)
	struct aha_softc *aha;
	int icnt, ocnt, wait;
	u_char *retval;
	unsigned opcode;
	u_char  args;
{
	unsigned *ic = &opcode;
	u_char oc;
	register i;
	int sts;

	/*
	 * multiply the wait argument by a big constant
	 * zero defaults to 1 sec..
	 * all wait loops are in 50uSec cycles
	 */
	if (wait)
		wait *= 20000;
	else
		wait = 20000;
	/*
	 * Wait for the adapter to go idle, unless it's one of
	 * the commands which don't need this
	 */
	if (opcode != AHA_MBX_INIT && opcode != AHA_START_SCSI) {
		i = 20000;	/*do this for upto about a second */
		while (--i) {
			sts = inb(AHA_CTRL_STAT_PORT);
			if (sts & AHA_IDLE)
				break;
			delay(50);
		}
		if (!i) {
			printf("%s: aha_cmd, host not idle(0x%x)\n",
				aha->sc_dev.dv_xname, sts);
			return ENXIO;
		}
	}
	/*
	 * Now that it is idle, if we expect output, preflush the
	 * queue feeding to us.
	 */
	if (ocnt) {
		while ((inb(AHA_CTRL_STAT_PORT)) & AHA_DF)
			inb(AHA_CMD_DATA_PORT);
	}
	/*
	 * Output the command and the number of arguments given
	 * for each byte, first check the port is empty.
	 */
	icnt++;
	/* include the command */
	while (icnt--) {
		sts = inb(AHA_CTRL_STAT_PORT);
		for (i = wait; i; i--) {
			sts = inb(AHA_CTRL_STAT_PORT);
			if (!(sts & AHA_CDF))
				break;
			delay(50);
		}
		if (!i) {
			if (opcode != AHA_INQUIRE)
				printf("%s: aha_cmd, cmd/data port full\n",
					aha->sc_dev.dv_xname);
			outb(AHA_CTRL_STAT_PORT, AHA_SRST);
			return ENXIO;
		}
		outb(AHA_CMD_DATA_PORT, (u_char) (*ic++));
	}
	/*
	 * If we expect input, loop that many times, each time,
	 * looking for the data register to have valid data
	 */
	while (ocnt--) {
		sts = inb(AHA_CTRL_STAT_PORT);
		for (i = wait; i; i--) {
			sts = inb(AHA_CTRL_STAT_PORT);
			if (sts & AHA_DF)
				break;
			delay(50);
		}
		if (!i) {
			if (opcode != AHA_INQUIRE)
				printf("%s: aha_cmd, cmd/data port empty %d\n",
					aha->sc_dev.dv_xname, ocnt);
			outb(AHA_CTRL_STAT_PORT, AHA_SRST);
			return ENXIO;
		}
		oc = inb(AHA_CMD_DATA_PORT);
		if (retval)
			*retval++ = oc;
	}
	/*
	 * Wait for the board to report a finised instruction
	 */
	i = 20000;
	while (--i) {
		sts = inb(AHA_INTR_PORT);
		if (sts & AHA_HACC)
			break;
		delay(50);
	}
	if (!i) {
		printf("%s: aha_cmd, host not finished(0x%x)\n",
			aha->sc_dev.dv_xname, sts);
		return ENXIO;
	}
	outb(AHA_CTRL_STAT_PORT, AHA_IRST);
	return 0;
}

/*
 * Check if the device can be found at the port given
 * and if so, set it up ready for further work
 * as an argument, takes the isa_device structure from
 * autoconf.c
 */
int
ahaprobe(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct aha_softc *aha = (void *)self;
	struct isa_attach_args *ia = aux;

#ifdef NEWCONFIG
	if (ia->ia_iobase == IOBASEUNK)
		return 0;
#endif

	aha->aha_base = ia->ia_iobase;

	/*
	 * Try initialise a unit at this location
	 * sets up dma and bus speed, loads aha->aha_int
	 */
	if (aha_find(aha) != 0)
		return 0;

	/*
	 * Calculate the xor product of the aha struct's
	 * physical and virtual address. This allows us
	 * to change addresses within the structure
	 * from physical to virtual easily, as long as
	 * the structure is less than 1 page in size.
	 * This is used to recognise CCBs which are in 
	 * this struct and which are refered to by the
	 * hardware using physical addresses.
	 * (assumes malloc returns a chunk that doesn't
	 * span pages)
	 * eventually use the hash table in aha1742.c
	 */
	aha->kv_phys_xor = (long int) aha ^ (KVTOPHYS(aha));

	/*
	 * If it's there, put in it's interrupt vectors and dma channel
	 */
	if (ia->ia_irq != IRQUNK) {
		if (ia->ia_irq != aha->aha_int) {
			printf("aha%d: irq mismatch; kernel configured %d != board configured %d\n",
				aha->sc_dev.dv_unit, ffs(ia->ia_irq) - 1,
				ffs(aha->aha_int) - 1);
			return 0;
		}
	} else
		ia->ia_irq = aha->aha_int;

	if (ia->ia_drq != DRQUNK) {
		if (ia->ia_drq != aha->aha_dma) {
			printf("aha%d: drq mismatch; kernel configured %d != board configured %d\n",
				aha->sc_dev.dv_unit, ia->ia_drq, aha->aha_dma);
			return 0;
		}
	} else
		ia->ia_drq = aha->aha_dma;

	ia->ia_msize = 0;
	ia->ia_iosize = 4;
	return 1;
}

ahaprint()
{

}

/*
 * Attach all the sub-devices we can find
 */
void
ahaattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct isa_attach_args *ia = aux;
	struct aha_softc *aha = (void *)self;

	if (ia->ia_drq != DRQUNK)
		isa_dmacascade(ia->ia_drq);

	aha_init(aha);

	/*
	 * fill in the prototype scsi_link.
	 */
	aha->sc_link.adapter_softc = aha;
	aha->sc_link.adapter_targ = aha->aha_scsi_dev;
	aha->sc_link.adapter = &aha_switch;
	aha->sc_link.device = &aha_dev;

	printf("\n");

#ifdef NEWCONFIG
	isa_establish(&aha->sc_id, &aha->sc_dev);
#endif
	aha->sc_ih.ih_fun = ahaintr;
	aha->sc_ih.ih_arg = aha;
	aha->sc_ih.ih_level = IPL_BIO;
	intr_establish(ia->ia_irq, &aha->sc_ih);

	/*
	 * ask the adapter what subunits are present
	 */
	config_found(self, &aha->sc_link, ahaprint);
}

/*
 * Return some information to the caller about the adapter and its
 * capabilities.
 */
u_int 
aha_adapter_info(aha)
	struct aha_softc *aha;
{

	return 2;	/* 2 outstanding requests at a time per device */
}

/*
 * Catch an interrupt from the adaptor
 */
int
ahaintr(aha)
	struct aha_softc *aha;
{
	struct aha_ccb *ccb;
	u_char stat;
	register i;

#ifdef	AHADEBUG
	printf("ahaintr ");
#endif /*AHADEBUG */
	/*
	 * First acknowlege the interrupt, Then if it's not telling about
	 * a completed operation just return. 
	 */
	stat = inb(AHA_INTR_PORT);
	outb(AHA_CTRL_STAT_PORT, AHA_IRST);
	if (!(stat & AHA_MBIF))
		return -1;	/* XXX */
#ifdef	AHADEBUG
	printf("mbxin ");
#endif /*AHADEBUG */
	/*
	 * If it IS then process the competed operation
	 */
	for (i = 0; i < AHA_MBX_SIZE; i++) {
		if (aha->aha_mbx.mbi[i].stat != AHA_MBI_FREE) {
			ccb = (struct aha_ccb *) PHYSTOKV(
			    (_3btol(aha->aha_mbx.mbi[i].ccb_addr)));

			if ((stat = aha->aha_mbx.mbi[i].stat) != AHA_MBI_OK) {
				switch (stat) {
				case AHA_MBI_ABORT:
#ifdef	AHADEBUG
					if (aha_debug)
						printf("abort");
#endif /*AHADEBUG */
					ccb->host_stat = AHA_ABORTED;
					break;

				case AHA_MBI_UNKNOWN:
					ccb = (struct aha_ccb *) 0;
#ifdef	AHADEBUG
					if (aha_debug)
						printf("unknown ccb for abort ");
#endif /*AHADEBUG */
					/* may have missed it */
					/* no such ccb known for abort */

				case AHA_MBI_ERROR:
					break;

				default:
					panic("Impossible mbxi status");

				}
#ifdef	AHADEBUG
				 if (aha_debug && ccb) {
					u_char *cp;
					cp = (u_char *) (&(ccb->scsi_cmd));
					printf("op=%x %x %x %x %x %x\n",
						cp[0], cp[1], cp[2],
						cp[3], cp[4], cp[5]);
					printf("stat %x for mbi[%d]\n",
						aha->aha_mbx.mbi[i].stat, i);
					printf("addr = 0x%x\n", ccb);
				}
#endif /*AHADEBUG */
			}
			if (ccb) {
				untimeout(aha_timeout, ccb);
				aha_done(aha, ccb);
			}
			aha->aha_mbx.mbi[i].stat = AHA_MBI_FREE;
		}
	}
	return 1;
}

/*
 * A ccb (and hence a mbx-out is put onto the 
 * free list.
 */
void
aha_free_ccb(aha, ccb, flags)
	struct aha_softc *aha;
	struct aha_ccb *ccb;
	int flags;
{
	int opri;

	if (!(flags & SCSI_NOMASK))
		opri = splbio();

	ccb->next = aha->aha_ccb_free;
	aha->aha_ccb_free = ccb;
	ccb->flags = CCB_FREE;
	/*
	 * If there were none, wake anybody waiting for one to come free,
	 * starting with queued entries.
	 */
	if (!ccb->next)
		wakeup((caddr_t)&aha->aha_ccb_free);

	if (!(flags & SCSI_NOMASK))
		splx(opri);
}

/*
 * Get a free ccb (and hence mbox-out entry)
 */
struct aha_ccb *
aha_get_ccb(aha, flags)
	struct aha_softc *aha;
	int flags;
{
	int opri;
	struct aha_ccb *rc;

	if (!(flags & SCSI_NOMASK))
		opri = splbio();
	/*
	 * If we can and have to, sleep waiting for one
	 * to come free
	 */
	while ((!(rc = aha->aha_ccb_free)) && (!(flags & SCSI_NOSLEEP)))
		tsleep((caddr_t)&aha->aha_ccb_free, PRIBIO, "ahaccb", 0);
	if (rc) {
		aha->aha_ccb_free = aha->aha_ccb_free->next;
		rc->flags = CCB_ACTIVE;
	}
	if (!(flags & SCSI_NOMASK))
		splx(opri);
	return rc;
}

/*
 * We have a ccb which has been processed by the
 * adaptor, now we look to see how the operation
 * went. Wake up the owner if waiting
 */
void
aha_done(aha, ccb)
	struct aha_softc *aha;
	struct aha_ccb *ccb;
{
	struct scsi_sense_data *s1, *s2;
	struct scsi_xfer *xs = ccb->xfer;

	SC_DEBUG(xs->sc_link, SDEV_DB2, ("aha_done\n"));
	/*
	 * Otherwise, put the results of the operation
	 * into the xfer and call whoever started it
	 */
	if (!(xs->flags & INUSE)) {
		printf("%s: exiting but not in use!\n", aha->sc_dev.dv_xname);
		Debugger();
	}
	if ((ccb->host_stat != AHA_OK || ccb->target_stat != SCSI_OK)
	    && (!(xs->flags & SCSI_ERR_OK))) {
		/*
		 * We have an error, that we cannot ignore.
		 */
		s1 = (struct scsi_sense_data *) (((char *) (&ccb->scsi_cmd)) +
			ccb->scsi_cmd_length);
		s2 = &(xs->sense);

		if (ccb->host_stat) {
			SC_DEBUG(xs->sc_link, SDEV_DB3, ("host err 0x%x\n",
				ccb->host_stat));
			switch (ccb->host_stat) {
			case AHA_ABORTED:
			case AHA_SEL_TIMEOUT:	/* No response */
				xs->error = XS_TIMEOUT;
				break;
			default:	/* Other scsi protocol messes */
				xs->error = XS_DRIVER_STUFFUP;
				printf("%s: host_stat %x\n",
					aha->sc_dev.dv_xname, ccb->host_stat);
			}
		} else {
			SC_DEBUG(xs->sc_link, SDEV_DB3, ("target err 0x%x\n",
				ccb->target_stat));
			switch (ccb->target_stat) {
			case 0x02:
				/* structure copy!!!!! */
				*s2 = *s1;
				xs->error = XS_SENSE;
				break;
			case 0x08:
				xs->error = XS_BUSY;
				break;
			default:
				printf("%s: target_stat %x\n",
					aha->sc_dev.dv_xname, ccb->target_stat);
				xs->error = XS_DRIVER_STUFFUP;
			}
		}
	} else {
		/* All went correctly  OR errors expected */
		xs->resid = 0;
	}
	xs->flags |= ITSDONE;
	aha_free_ccb(aha, ccb, xs->flags);
	scsi_done(xs);
}

/*
 * Find the board and find its irq/drq
 */
int
aha_find(aha)
	struct aha_softc *aha;
{
	u_char ad[3];
	volatile int i, sts;
	struct aha_config conf;
	struct aha_inquire inquire;
	struct aha_extbios extbios;

	/*
	 * reset board, If it doesn't respond, assume 
	 * that it's not there.. good for the probe
	 */

	outb(AHA_CTRL_STAT_PORT, AHA_HRST | AHA_SRST);

	for (i = AHA_RESET_TIMEOUT; i; i--) {
		sts = inb(AHA_CTRL_STAT_PORT);
		if (sts == (AHA_IDLE | AHA_INIT))
			break;
		delay(1000);	/* calibrated in msec */
	}
	if (!i) {
#ifdef	AHADEBUG
		if (aha_debug)
			printf("aha_find: No answer from adaptec board\n");
#endif /*AHADEBUG */
		return ENXIO;
	}

	/*
	 * Assume we have a board at this stage, do an adapter inquire
	 * to find out what type of controller it is.  If the command
	 * fails, we assume it's either a crusty board or an old 1542
	 * clone, and skip the board-specific stuff.
	 */
	if (aha_cmd(aha, 0, sizeof(inquire), 1, &inquire, AHA_INQUIRE)) {
		/*
		 * aha_cmd() already started the reset.  It's not clear we
		 * even need to bother here.
		 */
		for (i = AHA_RESET_TIMEOUT; i; i--) {
			sts = inb(AHA_CTRL_STAT_PORT);
			if (sts == (AHA_IDLE | AHA_INIT))
				break;
			delay(1000);
		}
		if (!i) {
#ifdef AHADEBUG
			printf("aha_init: soft reset failed\n");
#endif /* AHADEBUG */
			return ENXIO;
		}
#ifdef AHADEBUG
		printf("aha_init: inquire command failed\n");
#endif /* AHADEBUG */
		goto noinquire;
	}
#ifdef AHADEBUG
	printf("%s: inquire %x, %x, %x, %x\n",
		aha->sc_dev.dv_xname,
		inquire.boardid, inquire.spec_opts,
		inquire.revision_1, inquire.revision_2);
#endif	/* AHADEBUG */
	/*
	 * If we are a 1542C or 1542CF disable the extended bios so that the
	 * mailbox interface is unlocked.
	 * No need to check the extended bios flags as some of the
	 * extensions that cause us problems are not flagged in that byte.
	 */
	if (inquire.boardid == 0x43 || inquire.boardid == 0x44 ||
	    inquire.boardid == 0x45) {
		aha_cmd(aha, 0, sizeof(extbios), 0, &extbios, AHA_EXT_BIOS);
#ifdef	AHADEBUG
		printf("%s: extended bios flags %x\n", aha->sc_dev.dv_xname,
			extbios.flags);
#endif	/* AHADEBUG */
		printf("%s: 1542C/CF detected, unlocking mailbox\n",
			aha->sc_dev.dv_xname);
		aha_cmd(aha, 2, 0, 0, 0, AHA_MBX_ENABLE,
			0, extbios.mailboxlock);
	}
noinquire:

	/*
	 * setup dma channel from jumpers and save int
	 * level
	 */
	delay(1000);		/* for Bustek 545 */
	aha_cmd(aha, 0, sizeof(conf), 0, &conf, AHA_CONF_GET);
	switch (conf.chan) {
	case CHAN0:
		aha->aha_dma = 0;
		break;
	case CHAN5:
		aha->aha_dma = 5;
		break;
	case CHAN6:
		aha->aha_dma = 6;
		break;
	case CHAN7:
		aha->aha_dma = 7;
		break;
	default:
		printf("illegal dma setting %x\n", conf.chan);
		return EIO;
	}

	switch (conf.intr) {
	case INT9:
		aha->aha_int = IRQ9;
		break;
	case INT10:
		aha->aha_int = IRQ10;
		break;
	case INT11:
		aha->aha_int = IRQ11;
		break;
	case INT12:
		aha->aha_int = IRQ12;
		break;
	case INT14:
		aha->aha_int = IRQ14;
		break;
	case INT15:
		aha->aha_int = IRQ15;
		break;
	default:
		printf("illegal int setting %x\n", conf.intr);
		return EIO;
	}

	/* who are we on the scsi bus? */
	aha->aha_scsi_dev = conf.scsi_dev;

	/*
	 * Change the bus on/off times to not clash with other dma users.
	 */
	aha_cmd(aha, 1, 0, 0, 0, AHA_BUS_ON_TIME_SET, 7);
	aha_cmd(aha, 1, 0, 0, 0, AHA_BUS_OFF_TIME_SET, 4);

#ifdef TUNE_1542
#error XXX Must deal with configuring the DRQ channel if we do this.
	/*
	 * Initialize memory transfer speed
	 * Not compiled in by default because it breaks some machines 
	 */
	if (!aha_set_bus_speed(aha))
		return EIO;
#endif /* TUNE_1542 */

	return 0;
}

/*
 * Start the board, ready for normal operation
 */
void
aha_init(aha)
	struct aha_softc *aha;
{
	u_char ad[4];
	volatile int i;

	/*
	 * Initialize mail box 
	 */
	lto3b(KVTOPHYS(&aha->aha_mbx), ad);

	aha_cmd(aha, 4, 0, 0, 0, AHA_MBX_INIT,
		AHA_MBX_SIZE, ad[0], ad[1], ad[2]);

	/*
	 * link the ccb's with the mbox-out entries and
	 * into a free-list
	 * this is a kludge but it works
	 */
	for (i = 0; i < AHA_MBX_SIZE; i++) {
		aha->aha_ccb[i].next = aha->aha_ccb_free;
		aha->aha_ccb_free = &aha->aha_ccb[i];
		aha->aha_ccb_free->flags = CCB_FREE;
		aha->aha_ccb_free->mbx = &aha->aha_mbx.mbo[i];
		lto3b(KVTOPHYS(aha->aha_ccb_free), aha->aha_mbx.mbo[i].ccb_addr);
	}
}

void 
ahaminphys(bp)
	struct buf *bp;
{

/*      aha seems to explode with 17 segs (64k may require 17 segs) */
/*      on old boards so use a max of 16 segs if you have problems here */
	if (bp->b_bcount > ((AHA_NSEG - 1) << PGSHIFT))
		bp->b_bcount = ((AHA_NSEG - 1) << PGSHIFT);
}

/*
 * start a scsi operation given the command and the data address. Also needs
 * the unit, target and lu.
 */
int 
aha_scsi_cmd(xs)
	struct scsi_xfer *xs;
{
	struct scsi_link *sc_link = xs->sc_link;
	struct aha_softc *aha = sc_link->adapter_softc;
	struct aha_ccb *ccb;
	struct aha_scat_gath *sg;
	int seg;		/* scatter gather seg being worked on */
	int thiskv;
	int thisphys, nextphys;
	int bytes_this_seg, bytes_this_page, datalen, flags;
	struct iovec *iovp;
	int s;

	SC_DEBUG(sc_link, SDEV_DB2, ("aha_scsi_cmd\n"));
	/*
	 * get a ccb (mbox-out) to use. If the transfer
	 * is from a buf (possibly from interrupt time)
	 * then we can't allow it to sleep
	 */
	flags = xs->flags;
	if (!(ccb = aha_get_ccb(aha, flags))) {
		xs->error = XS_DRIVER_STUFFUP;
		return TRY_AGAIN_LATER;
	}
	if (ccb->mbx->cmd != AHA_MBO_FREE)
		printf("%s: MBO not free\n", aha->sc_dev.dv_xname);

	/*
	 * Put all the arguments for the xfer in the ccb
	 */
	ccb->xfer = xs;
	if (flags & SCSI_RESET) {
		ccb->opcode = AHA_RESET_CCB;
	} else {
		/* can't use S/G if zero length */
		ccb->opcode = (xs->datalen ? AHA_INIT_SCAT_GATH_CCB
					   : AHA_INITIATOR_CCB);
	}
	ccb->target = sc_link->target;
	ccb->data_out = 0;
	ccb->data_in = 0;
	ccb->lun = sc_link->lun;
	ccb->scsi_cmd_length = xs->cmdlen;
	ccb->req_sense_length = sizeof(ccb->scsi_sense);

	if ((xs->datalen) && (!(flags & SCSI_RESET))) {
		/* can use S/G only if not zero length */
		lto3b(KVTOPHYS(ccb->scat_gath), ccb->data_addr);
		sg = ccb->scat_gath;
		seg = 0;
#ifdef	TFS_ONLY
		if (flags & SCSI_DATA_UIO) {
			iovp = ((struct uio *) xs->data)->uio_iov;
			datalen = ((struct uio *) xs->data)->uio_iovcnt;
			while ((datalen) && (seg < AHA_NSEG)) {
				lto3b(iovp->iov_base, sg->seg_addr);
				lto3b(iovp->iov_len, sg->seg_len);
				SC_DEBUGN(sc_link, SDEV_DB4, ("UIO(0x%x@0x%x)",
					iovp->iov_len, iovp->iov_base));
				sg++;
				iovp++;
				seg++;
				datalen--;
			}
		} else
#endif /*TFS_ONLY */
		{
			/*
			 * Set up the scatter gather block
			 */

			SC_DEBUG(sc_link, SDEV_DB4,
				("%d @0x%x:- ", xs->datalen, xs->data));
			datalen = xs->datalen;
			thiskv = (int) xs->data;
			thisphys = KVTOPHYS(thiskv);

			while ((datalen) && (seg < AHA_NSEG)) {
				bytes_this_seg = 0;

				/* put in the base address */
				lto3b(thisphys, sg->seg_addr);

				SC_DEBUGN(sc_link, SDEV_DB4,
					("0x%x", thisphys));

				/* do it at least once */
				nextphys = thisphys;
				while ((datalen) && (thisphys == nextphys)) {
					/*
					 * This page is contiguous (physically)
					 * with the the last, just extend the
					 * length
					 */
					/* check it fits on the ISA bus */
					if (thisphys > 0xFFFFFF) {
						printf("%s: DMA beyond"
							" end of ISA\n",
							aha->sc_dev.dv_xname);
						xs->error = XS_DRIVER_STUFFUP;
						aha_free_ccb(aha, ccb, flags);
						return HAD_ERROR;
					}
					/** how far to the end of the page ***/
					nextphys = (thisphys & ~PGOFSET) + NBPG;
					bytes_this_page = nextphys - thisphys;
					/**** or the data ****/
					bytes_this_page = min(bytes_this_page,
							      datalen);
					bytes_this_seg += bytes_this_page;
					datalen -= bytes_this_page;

					/* get more ready for the next page */
					thiskv = (thiskv & ~PGOFSET) + NBPG;
					if (datalen)
						thisphys = KVTOPHYS(thiskv);
				}
				/*
				 * next page isn't contiguous, finish the seg
				 */
				SC_DEBUGN(sc_link, SDEV_DB4,
					("(0x%x)", bytes_this_seg));
				lto3b(bytes_this_seg, sg->seg_len);
				sg++;
				seg++;
			}
		}
		lto3b(seg * sizeof(struct aha_scat_gath), ccb->data_length);
		SC_DEBUGN(sc_link, SDEV_DB4, ("\n"));

		if (datalen) {
			/*
			 * there's still data, must have run out of segs!
			 */
			printf("%s: aha_scsi_cmd, more than %d DMA segs\n",
				aha->sc_dev.dv_xname, AHA_NSEG);
			xs->error = XS_DRIVER_STUFFUP;
			aha_free_ccb(aha, ccb, flags);
			return HAD_ERROR;
		}
	} else {		/* No data xfer, use non S/G values */
		lto3b(0, ccb->data_addr);
		lto3b(0, ccb->data_length);
	}
	lto3b(0, ccb->link_addr);

	/*
	 * Put the scsi command in the ccb and start it
	 */
	if (!(flags & SCSI_RESET))
		bcopy(xs->cmd, &ccb->scsi_cmd, ccb->scsi_cmd_length);
	if (!(flags & SCSI_NOMASK)) {
		s = splbio();	/* stop instant timeouts */
		timeout(aha_timeout, ccb, (xs->timeout * hz) / 1000);
		aha_startmbx(ccb->mbx);
		/*
		 * Usually return SUCCESSFULLY QUEUED
		 */
		splx(s);
		SC_DEBUG(sc_link, SDEV_DB3, ("sent\n"));
		return SUCCESSFULLY_QUEUED;
	}
	aha_startmbx(ccb->mbx);
	SC_DEBUG(sc_link, SDEV_DB3, ("cmd sent, waiting\n"));

	/*
	 * If we can't use interrupts, poll on completion
	 */
	return aha_poll(aha, xs, ccb);	/* only during boot */
}

/*
 * Poll a particular unit, looking for a particular xs
 */
int 
aha_poll(aha, xs, ccb)
	struct aha_softc *aha;
	struct scsi_xfer *xs;
	struct aha_ccb *ccb;
{
	int done = 0;
	int count = xs->timeout;
	u_char stat;

	/* timeouts are in msec, so we loop in 1000 usec cycles */
	while (count) {
		/*
		 * If we had interrupts enabled, would we
		 * have got an interrupt?
		 */
		stat = inb(AHA_INTR_PORT);
		if (stat & AHA_ANY_INTR)
			ahaintr(aha);
		if (xs->flags & ITSDONE)
			break;
		delay(1000);	/* only happens in boot so ok */
		count--;
	}
	if (!count) {
		/*
		 * We timed out, so call the timeout handler manually,
		 * accounting for the fact that the clock is not running yet
		 * by taking out the clock queue entry it makes.
		 */
		aha_timeout((caddr_t)ccb);

		/*
		 * because we are polling, take out the timeout entry
		 * aha_timeout made
		 */
		untimeout(aha_timeout, ccb);
		count = 2000;
		while (count) {
			/*
			 * Once again, wait for the int bit
			 */
			stat = inb(AHA_INTR_PORT);
			if (stat & AHA_ANY_INTR)
				ahaintr(aha);
			if (xs->flags & ITSDONE)
				break;
			delay(1000);	/* only happens in boot so ok */
			count--;
		}
		if (!count) {
			/*
			 * We timed out again.. this is bad
			 * Notice that this time there is no
			 * clock queue entry to remove
			 */
			aha_timeout((caddr_t)ccb);
		}
	}
	if (xs->error)
		return HAD_ERROR;
	return COMPLETE;

}

#ifdef TUNE_1542
/*
 * Try all the speeds from slowest to fastest.. if it finds a
 * speed that fails, back off one notch from the last working
 * speed (unless there is no other notch).
 * Returns the nSEC value of the time used
 * or 0 if it could get a working speed (or the NEXT speed 
 * failed)
 */
static struct bus_speed {
	u_char arg;
	int nsecs;
} aha_bus_speeds[] = {
	{0x88, 100},
	{0x99, 150},
	{0xaa, 200},
	{0xbb, 250},
	{0xcc, 300},
	{0xdd, 350},
	{0xee, 400},
	{0xff, 450}
};

int	
aha_set_bus_speed(aha)
	struct aha_softc *aha;
{
	int speed;
	int lastworking;

	lastworking = -1;
	for (speed = 7; speed >= 0; speed--) {
		if (!aha_bus_speed_check(aha, speed))
			break;
		lastworking = speed;
	}
	if (lastworking == -1) {
		printf("no working bus speed\n");
		return 0;
	}
	printf("%d nsec ", aha_bus_speeds[lastworking].nsecs);
	if (lastworking == 7)	/* is slowest already */
		printf("marginal\n");
	else {
		lastworking++;
		printf("ok, using %d nsec\n",
		    aha_bus_speeds[lastworking].nsecs);
	}
	if (!aha_bus_speed_check(aha, lastworking)) {
		printf("test retry failed.. aborting.\n");
		return 0;
	}
	return 1;
}

/*
 * Set the DMA speed to the Nth speed and try an xfer. If it
 * fails return 0, if it succeeds return the nSec value selected
 * If there is no such speed return HAD_ERROR.
 */
static char aha_test_string[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyz!@";

int 
aha_bus_speed_check(aha, speed)
	struct aha_softc *aha;
	int speed;
{
	int numspeeds = sizeof(aha_bus_speeds) / sizeof(struct bus_speed);
	int loopcount;
	u_char ad[3];

	/*
	 * Set the dma-speed
	 */
	aha_cmd(aha, 1, 0, 0, 0, AHA_SPEED_SET, aha_bus_speeds[speed].arg);

	/*
	 * put the test data into the buffer and calculate
	 * it's address. Read it onto the board
	 */
	for (loopcount = 100; loopcount; loopcount--) {
		lto3b(KVTOPHYS(aha_test_string), ad);
		aha_cmd(aha, 3, 0, 0, 0, AHA_WRITE_FIFO, ad[0], ad[1], ad[2]);

		/*
		 * Clear the buffer then copy the contents back from the
		 * board.
		 */
		bzero(aha_scratch_buf, 54);
	
		lto3b(KVTOPHYS(aha_scratch_buf), ad);
		aha_cmd(aha, 3, 0, 0, 0, AHA_READ_FIFO, ad[0], ad[1], ad[2]);
	
		/*
		 * Compare the original data and the final data and return the
		 * correct value depending upon the result.  We only check the
		 * first 54 bytes, because that's all the board copies during
		 * WRITE_FIFO and READ_FIFO.
		 */
		if (memcmp(aha_test_string, aha_scratch_buf, 54))
			return 0; /* failed test */
	}

	/* copy succeeded; assume speed ok */
	return 1;
}
#endif /* TUNE_1542 */

void
aha_timeout(arg)
	void *arg;
{
	int s = splbio();
	struct aha_ccb *ccb = (struct aha_ccb *)arg;
	struct aha_softc *aha;

	aha = ccb->xfer->sc_link->adapter_softc;
	sc_print_addr(ccb->xfer->sc_link);
	printf("timed out ");

	/*
	 * If The ccb's mbx is not free, then
	 * the board has gone south
	 */
	if (ccb->mbx->cmd != AHA_MBO_FREE) {
		printf("\nadapter not taking commands.. frozen?!\n");
		Debugger();
	}

	/*
	 * If it has been through before, then
	 * a previous abort has failed, don't
	 * try abort again
	 */
	if (ccb->flags == CCB_ABORTED) {
		/* abort timed out */
		printf("AGAIN\n");
		ccb->xfer->retries = 0;		/* I MEAN IT ! */
		ccb->host_stat = AHA_ABORTED;
		aha_done(aha, ccb);
	} else {
		/* abort the operation that has timed out */
		printf("\n");
		aha_abortmbx(ccb->mbx);
		/* 4 secs for the abort */
		timeout(aha_timeout, ccb, 2 * hz);
		ccb->flags = CCB_ABORTED;
	}
	splx(s);
}
