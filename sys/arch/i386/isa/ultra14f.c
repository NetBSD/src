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
 *      $Id: ultra14f.c,v 1.30.2.5 1994/08/22 21:55:28 mycroft Exp $
 */

/*
 * Ported for use with the UltraStor 14f by Gary Close (gclose@wvnvms.wvnet.edu)
 * Slight fixes to timeouts to run with the 34F
 * Thanks to Julian Elischer for advice and help with this port.
 *
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
 *
 * commenced: Sun Sep 27 18:14:01 PDT 1992
 * slight mod to make work with 34F as well: Wed Jun  2 18:05:48 WST 1993
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

#ifdef	DDB
int     Debugger();
#else	/* DDB */
#define Debugger()
#endif	/* DDB */

typedef struct {
	u_char addr[4];
} physaddr;
typedef struct {
	u_char len[4];
} physlen;

#define KVTOPHYS(x)   vtophys(x)

#define UHA_MSCP_MAX	32	/* store up to 32MSCPs at any one time
				 * MAX = ?
				 */
#define	MSCP_HASH_SIZE	32	/* when we have a physical addr. for
				 * a mscp and need to find the mscp in
				 * space, look it up in the hash table
				 */
#define	MSCP_HASH_SHIFT	9	/* only hash on multiples of 512 */
#define MSCP_HASH(x)	((((long)(x))>>MSCP_HASH_SHIFT) % MSCP_HASH_SIZE)

#define UHA_NSEG	33	/* number of dma segments supported */

/************************** board definitions *******************************/
/*
 * I/O Port Interface
 */
#define U14_LMASK		0x0000	/* local doorbell mask reg */
#define U14_LINT		0x0001	/* local doorbell int/stat reg */
#define U14_SMASK		0x0002	/* system doorbell mask reg */
#define U14_SINT		0x0003	/* system doorbell int/stat reg */
#define U14_ID			0x0004	/* product id reg (2 ports) */
#define U14_CONFIG		0x0006	/* config reg (2 ports) */
#define U14_OGMPTR		0x0008	/* outgoing mail ptr (4 ports) */
#define U14_ICMPTR		0x000c	/* incoming mail ptr (4 ports) */

#define	U24_CONFIG		0x0c85	/* config reg (3 ports) */
#define	U24_LMASK		0x0c8c	/* local doorbell mask reg */
#define	U24_LINT		0x0c8d	/* local doorbell int/stat reg */
#define	U24_SMASK		0x0c8e	/* system doorbell mask reg */
#define	U24_SINT		0x0c8f	/* system doorbell int/stat reg */
#define	U24_OGMCMD		0x0c96	/* outgoing commands */
#define	U24_OGMPTR		0x0c97	/* outgoing mail ptr (4 ports) */
#define	U24_ICMCMD		0x0c9b	/* incoming commands */
#define	U24_ICMPTR		0x0c9c	/* incoming mail ptr (4 ports) */

/*
 * UHA_LMASK bits (read only) 
 */
#define UHA_LDIE		0x80	/* local doorbell int enabled */
#define UHA_SRSTE		0x40	/* soft reset enabled */
#define UHA_ABORTEN		0x10	/* abort MSCP enabled */
#define UHA_OGMINTEN		0x01	/* outgoing mail interrupt enabled */

/*
 * UHA_LINT bits (read)
 */
#define U14_LDIP		0x80	/* local doorbell int pending */

#define	U24_LDIP		0x02	/* local doorbell int pending */

/*
 * UHA_LINT bits (write)
 */
#define U14_OGMINT		0x01	/* tell adapter to get mail */
#define U14_ABORT		0x10	/* abort MSCP */
#define U14_SBRST		0x20	/* scsi bus reset */
#define U14_ADRST		0x40	/* adapter soft reset */
#define U14_ASRST		0x60	/* adapter and scsi reset */

#define	U24_OGMINT		0x02	/* tell adapter to get mail */
#define	U24_ABORT		0x10	/* abort MSCP */
#define	U24_SBRST		0x40	/* scsi bus reset */
#define	U24_ADRST		0x80	/* adapter soft reset */
#define	U24_ASRST		0xc0	/* adapter and scsi reset */

/*
 * UHA_SMASK bits (read)
 */
#define UHA_SINTEN		0x80	/* system doorbell interupt Enabled */
#define UHA_ABORT_COMPLETE_EN   0x10	/* abort MSCP command complete int Enabled */
#define UHA_ICM_ENABLED		0x01	/* ICM interrupt enabled */

/*
 * UHA_SMASK bits (write)
 */
#define UHA_ENSINT		0x80	/* enable system doorbell interrupt */
#define UHA_EN_ABORT_COMPLETE   0x10	/* enable abort MSCP complete int */
#define UHA_ENICM		0x01	/* enable ICM interrupt */

/*
 * UHA_SINT bits (read)
 */
#define U14_ABORT_SUCC		0x10	/* abort MSCP successful */
#define U14_ABORT_FAIL		0x18	/* abort MSCP failed */
#define U14_SINTP		0x80	/* system doorbell int pending */

#define	U24_SINTP		0x02	/* system doorbell int pending */
#define	U24_ABORT_SUCC		0x10	/* abort MSCP successful */
#define	U24_ABORT_FAIL		0x18	/* abort MSCP failed */

/*
 * UHA_SINT bits (write)
 */
#define U14_ICM_ACK		0x01	/* acknowledge ICM and clear */
#define U14_ABORT_ACK		0x18	/* acknowledge status and clear */

#define	U24_ICM_ACK		0x02	/* acknowledge ICM and clear */
#define	U24_ABORT_ACK		0x18	/* acknowledge status and clear */

/* 
 * U14_CONFIG bits (read only)
 */
#define U14_DMA_CH5		0x0000	/* DMA channel 5 */
#define U14_DMA_CH6		0x4000	/* 6 */
#define U14_DMA_CH7		0x8000	/* 7 */
#define	U14_DMA_MASK		0xc000
#define U14_IRQ15		0x0000	/* IRQ 15 */
#define U14_IRQ14		0x1000	/* 14 */
#define U14_IRQ11		0x2000	/* 11 */
#define U14_IRQ10		0x3000	/* 10 */
#define	U14_IRQ_MASK		0x3000
#define	U14_HOSTID_MASK		0x0007

/*
 * U24_CONFIG bits (read only)
 */
#define	U24_MAGIC1		0x08
#define	U24_IRQ15		0x10
#define	U24_IRQ14		0x20
#define	U24_IRQ11		0x40
#define	U24_IRQ10		0x80
#define	U24_IRQ_MASK		0xf0

#define	U24_MAGIC2		0x04

#define	U24_HOSTID_MASK		0x07

/*
 * EISA registers (offset from slot base)
 */
#define	EISA_VENDOR		0x0c80	/* vendor ID (2 ports) */
#define	EISA_MODEL		0x0c82	/* model number (2 ports) */
#define	EISA_CONTROL		0x0c84
#define	 EISA_RESET		0x04
#define	 EISA_ERROR		0x02
#define	 EISA_ENABLE		0x01

/*
 * ha_status error codes
 */
#define UHA_NO_ERR		0x00	/* No error supposedly */
#define UHA_SBUS_ABORT_ERR	0x84	/* scsi bus abort error */
#define UHA_SBUS_TIMEOUT	0x91	/* scsi bus selection timeout */
#define UHA_SBUS_OVER_UNDER	0x92	/* scsi bus over/underrun */
#define UHA_BAD_SCSI_CMD	0x96	/* illegal scsi command */
#define UHA_AUTO_SENSE_ERR	0x9b	/* auto request sense err */
#define UHA_SBUS_RES_ERR	0xa3	/* scsi bus reset error */
#define UHA_BAD_SG_LIST		0xff	/* invalid scatter gath list */

struct uha_dma_seg {
	physaddr addr;
	physlen len;
};

struct mscp {
	u_char opcode:3;
#define U14_HAC		0x01	/* host adapter command */
#define U14_TSP		0x02	/* target scsi pass through command */
#define U14_SDR		0x04	/* scsi device reset */
	u_char xdir:2;		/* xfer direction */
#define U14_SDET	0x00	/* determined by scsi command */
#define U14_SDIN	0x01	/* scsi data in */
#define U14_SDOUT	0x02	/* scsi data out */
#define U14_NODATA	0x03	/* no data xfer */
	u_char dcn:1;		/* disable disconnect for this command */
	u_char ca:1;		/* cache control */
	u_char sgth:1;		/* scatter gather flag */
	u_char target:3;
	u_char chan:2;		/* scsi channel (always 0 for 14f) */
	u_char lun:3;
	physaddr data;
	physlen datalen;
	physaddr link;
	u_char link_id;
	u_char sg_num;		/*number of scat gath segs */
	/*in s-g list if sg flag is */
	/*set. starts at 1, 8bytes per */
	u_char senselen;
	u_char cdblen;
	u_char cdb[12];
	u_char ha_status;
	u_char targ_status;
	physaddr sense;		/* if 0 no auto sense */
	/*-----------------end of hardware supported fields----------------*/
	struct mscp *next;	/* in free list */
	struct scsi_xfer *xs;	/* the scsi_xfer for this cmd */
	int flags;
#define MSCP_FREE	0
#define MSCP_ACTIVE	1
#define MSCP_ABORTED	2
	struct uha_dma_seg uha_dma[UHA_NSEG];
	struct scsi_sense_data mscp_sense;
	struct mscp *nexthash;
	long hashkey;
};

struct uha_softc {
	struct device sc_dev;
	struct isadev sc_id;
	struct intrhand sc_ih;

	u_short sc_iobase;

	void (*send_mbox)();
	int (*abort)();
	int (*poll)();
	int (*intr)();
	void (*init)();

	struct mscp *mscphash[MSCP_HASH_SIZE];
	struct mscp *free_mscp;
	u_short uha_int;
	u_short uha_dma;
	int uha_scsi_dev;		/* our scsi id */
	int nummscps;
	struct scsi_link sc_link;
};

void u14_send_mbox __P((struct uha_softc *, struct mscp *));
void u24_send_mbox __P((struct uha_softc *, struct mscp *));
int u14_abort __P((struct uha_softc *, struct mscp *));
int u24_abort __P((struct uha_softc *, struct mscp *));
int u14_poll __P((struct uha_softc *, int));
int u24_poll __P((struct uha_softc *, int));
u_int uha_adapter_info __P((struct uha_softc *));
int u14intr __P((struct uha_softc *));
int u24intr __P((struct uha_softc *));
void uha_done __P((struct uha_softc *, struct mscp *));
void uha_free_mscp __P((struct uha_softc *, struct mscp *, int flags));
struct mscp *uha_get_mscp __P((struct uha_softc *, int));
struct mscp *uha_mscp_phys_kv __P((struct uha_softc *, u_long));
int u14_find __P((struct uha_softc *, struct isa_attach_args *));
int u24_find __P((struct uha_softc *, struct isa_attach_args *));
void u14_init __P((struct uha_softc *));
void u24_init __P((struct uha_softc *));
void uhaminphys __P((struct buf *));
int uha_scsi_cmd __P((struct scsi_xfer *));
void uha_timeout __P((void *arg));
#ifdef UHADEBUG
void uha_print_mscp __P((struct mscp *));
void uha_print_active_mscp __P((struct uha_softc *));
#endif

u_long	scratch;
#define UHA_SHOWMSCPS 0x01
#define UHA_SHOWINTS 0x02
#define UHA_SHOWCMDS 0x04
#define UHA_SHOWMISC 0x08

struct scsi_adapter uha_switch = {
	uha_scsi_cmd,
	uhaminphys,
	0,
	0,
	uha_adapter_info,
	"uha"
};

/* the below structure is so we have a default dev struct for out link struct */
struct scsi_device uha_dev = {
	NULL,			/* Use default error handler */
	NULL,			/* have a queue, served by this */
	NULL,			/* have no async handler */
	NULL,			/* Use default 'done' routine */
	"uha",
	0
};

int uhaprobe();
void uhaattach();

struct cfdriver uhacd = {
	NULL, "uha", uhaprobe, uhaattach, DV_DULL, sizeof(struct uha_softc)
};

/*
 * Function to send a command out through a mailbox
 */
void
u14_send_mbox(uha, mscp)
	struct uha_softc *uha;
	struct mscp *mscp;
{
	u_short iobase = uha->sc_iobase;
	int spincount = 100000;	/* 1s should be enough */
	int s = splbio();

	while (--spincount) {
		if ((inb(iobase + U14_LINT) & U14_LDIP) == 0)
			break;
		delay(100);
	}
	if (!spincount) {
		printf("%s: uha_send_mbox, board not responding\n",
			uha->sc_dev.dv_xname);
		Debugger();
	}

	outl(iobase + U14_OGMPTR, KVTOPHYS(mscp));
	outb(iobase + U14_LINT, U14_OGMINT);

	splx(s);
}

void
u24_send_mbox(uha, mscp)
	struct uha_softc *uha;
	struct mscp *mscp;
{
	u_short iobase = uha->sc_iobase;
	int spincount = 100000;	/* 1s should be enough */
	int s = splbio();

	while (--spincount) {
		if ((inb(iobase + U24_LINT) & U24_LDIP) == 0)
			break;
		delay(100);
	}
	if (!spincount) {
		printf("%s: uha_send_mbox, board not responding\n",
			uha->sc_dev.dv_xname);
		Debugger();
	}

	outl(iobase + U24_OGMPTR, KVTOPHYS(mscp));
	outb(iobase + U24_OGMCMD, 1);
	outb(iobase + U24_LINT, U24_OGMINT);

	splx(s);
}

/*
 * Function to send abort
 */
int
u14_abort(uha, mscp)
	struct uha_softc *uha;
	struct mscp *mscp;
{
	u_short iobase = uha->sc_iobase;
	int spincount = 100;		/* 1 mSec */
	int abortcount = 200000;	/* 2 secs */
	int s = splbio();

	while (--spincount) {
		if ((inb(iobase + U14_LINT) & U14_LDIP) == 0)
			break;
		delay(10);
	}
	if (!spincount) {
		printf("%s: uha_abort, board not responding\n",
			uha->sc_dev.dv_xname);
		Debugger();
	}

	outl(iobase + U14_OGMPTR, KVTOPHYS(mscp));
	outb(iobase + U14_LINT, U14_ABORT);

	while (--abortcount) {
		if (inb(iobase + U14_SINT) & U14_ABORT_FAIL)
			break;
		delay(10);
	}
	if (!abortcount) {
		printf("%s: uha_abort, board not responding\n",
			uha->sc_dev.dv_xname);
		Debugger();
	}

	if ((inb(iobase + U14_SINT) & (U14_ABORT_FAIL | U14_ABORT_SUCC)) ==
	    U14_ABORT_SUCC) {
		outb(iobase + U14_SINT, U14_ABORT_ACK);
		splx(s);
		return 1;
	} else {
		outb(iobase + U14_SINT, U14_ABORT_ACK);
		splx(s);
		return 0;
	}
}

int
u24_abort(uha, mscp)
	struct uha_softc *uha;
	struct mscp *mscp;
{
	u_short iobase = uha->sc_iobase;
	int spincount = 100;		/* 1 mSec */
	int abortcount = 200000;	/* 2 secs */
	int s = splbio();

	while (--spincount) {
		if ((inb(iobase + U24_LINT) & U24_LDIP) == 0)
			break;
		delay(10);
	}
	if (!spincount) {
		printf("%s: uha_abort, board not responding\n",
			uha->sc_dev.dv_xname);
		Debugger();
	}

	outl(iobase + U24_OGMPTR, KVTOPHYS(mscp));
	outb(iobase + U24_OGMCMD, 1);
	outb(iobase + U24_LINT, U24_ABORT);

	while (--abortcount) {
		if (inb(iobase + U24_SINT) & U24_ABORT_FAIL)
			break;
		delay(10);
	}
	if (!abortcount) {
		printf("%s: uha_abort, board not responding\n",
			uha->sc_dev.dv_xname);
		Debugger();
	}

	if ((inb(iobase + U24_SINT) & (U24_ABORT_FAIL | U24_ABORT_SUCC)) ==
	    U24_ABORT_SUCC) {
		outb(iobase + U24_SINT, U24_ABORT_ACK);
		splx(s);
		return 1;
	} else {
		outb(iobase + U24_SINT, U24_ABORT_ACK);
		splx(s);
		return 0;
	}
}

/*
 * Function to poll for command completion when in poll mode.
 *
 *	wait = timeout in msec
 */
int
u14_poll(uha, wait)
	struct uha_softc *uha;
	int wait;
{
	u_short iobase = uha->sc_iobase;

	while (--wait) {
		if (inb(iobase + U14_SINT) & U14_SINTP)
			break;
		delay(1000);	/* 1 mSec per loop */
	}
	if (!wait) {
		printf("%s: uha_poll, board not responding\n",
			uha->sc_dev.dv_xname);
		return EIO;
	}

	u14intr(uha);
	return 0;
}

int
u24_poll(uha, wait)
	struct uha_softc *uha;
	int wait;
{
	u_short iobase = uha->sc_iobase;

	while (--wait) {
		if (inb(iobase + U24_SINT) & U24_SINTP)
			break;
		delay(1000);	/* 1 mSec per loop */
	}
	if (!wait) {
		printf("%s: uha_poll, board not responding\n",
			uha->sc_dev.dv_xname);
		return EIO;
	}

	u24intr(uha);
	return 0;
}

/*
 * Check if the device can be found at the port given and if so, set it up
 * ready for further work as an argument, takes the isa_device structure
 * from autoconf.c
 */
int
uhaprobe(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct uha_softc *uha = (void *)self;
	struct isa_attach_args *ia = aux;

	uha->sc_iobase = ia->ia_iobase;

	/*
	 * Try initialise a unit at this location
	 * sets up dma and bus speed, loads uha->uha_int
	 */
	if (u24_find(uha, ia) != 0 && u14_find(uha, ia) != 0)
		return 0;

	if (ia->ia_irq != IRQUNK) {
		if (ia->ia_irq != uha->uha_int) {
			printf("uha%d: irq mismatch; kernel configured %d != board configured %d\n",
				uha->sc_dev.dv_unit, ffs(ia->ia_irq) - 1,
				ffs(uha->uha_int) - 1);
			return 0;
		}
	} else
		ia->ia_irq = uha->uha_int;

	if (ia->ia_drq != DRQUNK) {
		if (ia->ia_drq != uha->uha_dma) {
			printf("uha%d: drq mismatch; kernel configured %d != board configured %d\n",
				uha->sc_dev.dv_unit, ia->ia_drq, uha->uha_dma);
			return 0;
		}
	} else
		ia->ia_drq = uha->uha_dma;

	ia->ia_msize = 0;
	ia->ia_iosize = 16;
	return 1;
}

uhaprint()
{

}

/*
 * Attach all the sub-devices we can find
 */
void
uhaattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct isa_attach_args *ia = aux;
	struct uha_softc *uha = (void *)self;

	if (ia->ia_drq != DRQUNK)
		isa_dmacascade(ia->ia_drq);

	(uha->init)(uha);

	/*
	 * fill in the prototype scsi_link.
	 */
	uha->sc_link.adapter_softc = uha;
	uha->sc_link.adapter_targ = uha->uha_scsi_dev;
	uha->sc_link.adapter = &uha_switch;
	uha->sc_link.device = &uha_dev;

	printf("\n");

#ifdef NEWCONFIG
	isa_establish(&uha->sc_id, &uha->sc_dev);
#endif
	uha->sc_ih.ih_fun = uha->intr;
	uha->sc_ih.ih_arg = uha;
	uha->sc_ih.ih_level = IPL_BIO;
	intr_establish(ia->ia_irq, &uha->sc_ih);

	/*
	 * ask the adapter what subunits are present
	 */
	config_found(self, &uha->sc_link, uhaprint);
}

/*
 * Return some information to the caller about
 * the adapter and it's capabilities
 */
u_int
uha_adapter_info(uha)
	struct uha_softc *uha;
{

	return 2;	/* 2 outstanding requests at a time per device */
}

/*
 * Catch an interrupt from the adaptor
 */
int
u14intr(uha)
	struct uha_softc *uha;
{
	struct mscp *mscp;
	u_char uhastat;
	u_long mboxval;
	u_short iobase = uha->sc_iobase;

#ifdef	UHADEBUG
	printf("%s: uhaintr ", uha->sc_dev.dv_xname);
#endif /*UHADEBUG */

	if ((inb(iobase + U14_SINT) & U14_SINTP) == 0)
		return 0;

	do {
		/*
		 * First get all the information and then
		 * acknowledge the interrupt
		 */
		uhastat = inb(iobase + U14_SINT);
		mboxval = inl(iobase + U14_ICMPTR);
		outb(iobase + U14_SINT, U14_ICM_ACK);

#ifdef	UHADEBUG
		printf("status = 0x%x ", uhastat);
#endif /*UHADEBUG*/

		/*
		 * Process the completed operation
		 */
		mscp = uha_mscp_phys_kv(uha, mboxval);
		if (!mscp) {
			printf("uha: BAD MSCP RETURNED\n");
			return 0;	/* whatever it was, it'll timeout */
		}
		untimeout(uha_timeout, mscp);

		uha_done(uha, mscp);
	} while (inb(iobase + U14_SINT) & U14_SINTP);

	return 1;
}

int
u24intr(uha)
	struct uha_softc *uha;
{
	struct mscp *mscp;
	u_char uhastat;
	u_long mboxval;
	u_short iobase = uha->sc_iobase;

#ifdef	UHADEBUG
	printf("%s: uhaintr ", uha->sc_dev.dv_xname);
#endif /*UHADEBUG */

	if ((inb(iobase + U24_SINT) & U24_SINTP) == 0)
		return 0;

	do {
		/*
		 * First get all the information and then
		 * acknowledge the interrupt
		 */
		uhastat = inb(iobase + U24_SINT);
		mboxval = inl(iobase + U24_ICMPTR);
		outb(iobase + U24_SINT, U24_ICM_ACK);
		outb(iobase + U24_ICMCMD, 0);

#ifdef	UHADEBUG
		printf("status = 0x%x ", uhastat);
#endif /*UHADEBUG*/

		/*
		 * Process the completed operation
		 */
		mscp = uha_mscp_phys_kv(uha, mboxval);
		if (!mscp) {
			printf("uha: BAD MSCP RETURNED\n");
			return 0;	/* whatever it was, it'll timeout */
		}
		untimeout(uha_timeout, mscp);

		uha_done(uha, mscp);
	} while (inb(iobase + U24_SINT) & U24_SINTP);

	return 1;
}

/*
 * We have a mscp which has been processed by the adaptor, now we look to see
 * how the operation went.
 */
void
uha_done(uha, mscp)
	struct uha_softc *uha;
	struct mscp *mscp;
{
	struct scsi_sense_data *s1, *s2;
	struct scsi_xfer *xs = mscp->xs;

	SC_DEBUG(xs->sc_link, SDEV_DB2, ("uha_done\n"));
	/*
	 * Otherwise, put the results of the operation
	 * into the xfer and call whoever started it
	 */
	if ((mscp->ha_status == UHA_NO_ERR) || (xs->flags & SCSI_ERR_OK)) {
		/* all went correctly OR errors expected */
		xs->resid = 0;
		xs->error = 0;
	} else {
		s1 = &mscp->mscp_sense;
		s2 = &xs->sense;

		if (mscp->ha_status != UHA_NO_ERR) {
			switch (mscp->ha_status) {
			case UHA_SBUS_TIMEOUT:		/* No response */
				SC_DEBUG(xs->sc_link, SDEV_DB3,
					("timeout reported back\n"));
				xs->error = XS_TIMEOUT;
				break;
			case UHA_SBUS_OVER_UNDER:
				SC_DEBUG(xs->sc_link, SDEV_DB3,
					("scsi bus xfer over/underrun\n"));
				xs->error = XS_DRIVER_STUFFUP;
				break;
			case UHA_BAD_SG_LIST:
				SC_DEBUG(xs->sc_link, SDEV_DB3,
					("bad sg list reported back\n"));
				xs->error = XS_DRIVER_STUFFUP;
				break;
			default:	/* Other scsi protocol messes */
				xs->error = XS_DRIVER_STUFFUP;
				SC_DEBUG(xs->sc_link, SDEV_DB3,
					("unexpected ha_status: %x\n",
					mscp->ha_status));
			}
		} else {
			if (mscp->targ_status != 0) {
				/*
				 * I have no information for any possible value
				 * of target status field other than 0 means no
				 * error!!  So I guess any error is unexpected
				 * in that event!!
				 */
				SC_DEBUG(xs->sc_link, SDEV_DB3,
					("unexpected targ_status: %x\n",
					mscp->targ_status));
				xs->error = XS_DRIVER_STUFFUP;
			}
		}
	}
    done:
	xs->flags |= ITSDONE;
	uha_free_mscp(uha, mscp, xs->flags);
	scsi_done(xs);
}

/*
 * A mscp (and hence a mbx-out) is put onto the free list.
 */
void
uha_free_mscp(uha, mscp, flags)
	struct uha_softc *uha;
	struct mscp *mscp;
	int flags;
{
	int opri;

	if (!(flags & SCSI_NOMASK))
		opri = splbio();

	mscp->next = uha->free_mscp;
	uha->free_mscp = mscp;
	mscp->flags = MSCP_FREE;

	/*
	 * If there were none, wake abybody waiting for
	 * one to come free, starting with queued entries
	 */
	if (!mscp->next)
		wakeup((caddr_t)&uha->free_mscp);

	if (!(flags & SCSI_NOMASK))
		splx(opri);
}

/*
 * Get a free mscp
 *
 * If there are none, see if we can allocate a new one.  If so, put it in the
 * hash table too otherwise either return an error or sleep.
 */
struct mscp *
uha_get_mscp(uha, flags)
	struct uha_softc *uha;
	int flags;
{
	int opri;
	struct mscp *mscpp;
	int hashnum;

	if (!(flags & SCSI_NOMASK))
		opri = splbio();

	/*
	 * If we can and have to, sleep waiting for one to come free
	 * but only if we can't allocate a new one
	 */
	while (!(mscpp = uha->free_mscp)) {
		if (uha->nummscps < UHA_MSCP_MAX) {
			if (mscpp = (struct mscp *)malloc(sizeof(struct mscp),
			    M_TEMP, M_NOWAIT)) {
				bzero(mscpp, sizeof(struct mscp));
				uha->nummscps++;
				mscpp->flags = MSCP_ACTIVE;
				/*
				 * put in the phystokv hash table
				 * Never gets taken out.
				 */
				mscpp->hashkey = KVTOPHYS(mscpp);
				hashnum = MSCP_HASH(mscpp->hashkey);
				mscpp->nexthash = uha->mscphash[hashnum];
				uha->mscphash[hashnum] = mscpp;
			} else {
				printf("%s: Can't malloc MSCP\n",
					uha->sc_dev.dv_xname);
			}
			goto gottit;
		} else {
			if (!(flags & SCSI_NOSLEEP))
				tsleep((caddr_t)&uha->free_mscp, PRIBIO,
					"uhamsc", 0);
		}
	}

	if (mscpp) {
		/* Get MSCP from the free list */
		uha->free_mscp = mscpp->next;
		mscpp->flags = MSCP_ACTIVE;
	}

    gottit:
      	if (!(flags & SCSI_NOMASK))
		splx(opri);

	return mscpp;
}

/*
 * given a physical address, find the mscp that it corresponds to.
 */
struct mscp *
uha_mscp_phys_kv(uha, mscp_phys)
	struct uha_softc *uha;
	u_long mscp_phys;
{
	int hashnum = MSCP_HASH(mscp_phys);
	struct mscp *mscpp = uha->mscphash[hashnum];

	while (mscpp) {
		if (mscpp->hashkey == mscp_phys)
			break;
		mscpp = mscpp->nexthash;
	}
	return mscpp;
}

/*
 * Start the board, ready for normal operation
 */
int
u14_find(uha, ia)
	struct uha_softc *uha;
	struct isa_attach_args *ia;
{
	u_short iobase = uha->sc_iobase;
	u_short model, config;
	int resetcount = 4000;	/* 4 secs? */

	if (ia->ia_iobase == IOBASEUNK)
		return ENXIO;

	model = htons(inw(iobase + U14_ID));
	if ((model & 0xfff0) != 0x5640)
		return ENXIO;

	config = htons(inw(iobase + U14_CONFIG));

	switch (model & 0x000f) {
	case 0x0001:
		/* This is a 34f, and doens't need an ISA DMA channel. */
		uha->uha_dma = DRQUNK;
		break;
	default:
		switch (config & U14_DMA_MASK) {
		case U14_DMA_CH5:
			uha->uha_dma = 5;
			break;
		case U14_DMA_CH6:
			uha->uha_dma = 6;
			break;
		case U14_DMA_CH7:
			uha->uha_dma = 7;
			break;
		default:
			printf("illegal dma setting %x\n", config & U14_DMA_MASK);
			return EIO;
		}
		break;
	}

	switch (config & U14_IRQ_MASK) {
	case U14_IRQ10:
		uha->uha_int = IRQ10;
		break;
	case U14_IRQ11:
		uha->uha_int = IRQ11;
		break;
	case U14_IRQ14:
		uha->uha_int = IRQ14;
		break;
	case U14_IRQ15:
		uha->uha_int = IRQ15;
		break;
	default:
		printf("illegal int setting %x\n", config & U14_IRQ_MASK);
		return EIO;
	}

	/* who are we on the scsi bus */
	uha->uha_scsi_dev = config & U14_HOSTID_MASK;

	outb(iobase + U14_LINT, U14_ASRST);

	while (--resetcount) {
		if (inb(iobase + U14_LINT))
			break;
		delay(1000);	/* 1 mSec per loop */
	}
	if (!resetcount) {
		printf("%s: board timed out during reset\n",
			uha->sc_dev.dv_xname);
		return ENXIO;
	}

	/* Save function pointers for later use. */
	uha->send_mbox = u14_send_mbox;
	uha->abort = u14_abort;
	uha->poll = u14_poll;
	uha->intr = u14intr;
	uha->init = u14_init;

	return 0;
}

int
u24_find(uha, ia)
	struct uha_softc *uha;
	struct isa_attach_args *ia;
{
	static int uha_slot = 0;
	u_short iobase;
	u_short vendor, model;
	u_char config0, config1, config2;
	u_char irq_ch, uha_id;
	int resetcount = 4000;	/* 4 secs? */

	if (ia->ia_iobase != IOBASEUNK)
		return ENXIO;

	while (uha_slot < 15) {
		uha_slot++;
		iobase = 0x1000 * uha_slot;
		
		vendor = htons(inw(iobase + EISA_VENDOR));
		if (vendor != 0x5663)	/* `USC' */
			continue;

		model = htons(inw(iobase + EISA_MODEL));
		if ((model & 0xfff0) != 0x0240) {
#ifndef trusted
			printf("u24_find: ignoring model %04x\n", model);
#endif
			continue;
		}

#if 0
		outb(iobase + EISA_CONTROL, EISA_ENABLE | EISA_RESET);
		delay(10);
		outb(iobase + EISA_CONTROL, EISA_ENABLE);
		/* Wait for reset? */
		delay(1000);
#endif

		config0 = inb(iobase + U24_CONFIG);
		config1 = inb(iobase + U24_CONFIG + 1);
		config2 = inb(iobase + U24_CONFIG + 2);
		if ((config0 & U24_MAGIC1) == 0 ||
		    (config1 & U24_MAGIC2) == 0)
			continue;

		irq_ch = config0 & U24_IRQ_MASK;
		uha_id = config2 & U24_HOSTID_MASK;

		uha->uha_dma = DRQUNK;

		switch (irq_ch) {
		case U24_IRQ10:
			uha->uha_int = IRQ10;
			break;
		case U24_IRQ11:
			uha->uha_int = IRQ11;
			break;
		case U24_IRQ14:
			uha->uha_int = IRQ14;
			break;
		case U24_IRQ15:
			uha->uha_int = IRQ15;
			break;
		default:
			printf("illegal int setting %x\n", irq_ch);
			continue;
		}

		/* who are we on the scsi bus */
		uha->uha_scsi_dev = uha_id;

		outb(iobase + U24_LINT, U24_ASRST);

		while (--resetcount) {
			if (inb(iobase + U24_LINT))
				break;
			delay(1000);	/* 1 mSec per loop */
		}
		if (!resetcount) {
			printf("%s: board timed out during reset\n",
				uha->sc_dev.dv_xname);
			continue;
		}

		/* Save function pointers for later use. */
		uha->send_mbox = u24_send_mbox;
		uha->abort = u24_abort;
		uha->poll = u24_poll;
		uha->intr = u24intr;
		uha->init = u24_init;

		return 0;
	}

	return ENXIO;
}

void
u14_init(uha)
	struct uha_softc *uha;
{
	u_short iobase = uha->sc_iobase;

	/* make sure interrupts are enabled */
	outb(iobase + U14_SMASK, UHA_SINTEN | UHA_ICM_ENABLED);
}

void
u24_init(uha)
	struct uha_softc *uha;
{
	u_short iobase = uha->sc_iobase;

	/* make sure interrupts are enabled */
	outb(iobase + U24_SMASK, 0xc2);		/* XXXX */
}

void
uhaminphys(bp)
	struct buf *bp;
{

	if (bp->b_bcount > ((UHA_NSEG - 1) << PGSHIFT))
		bp->b_bcount = ((UHA_NSEG - 1) << PGSHIFT);
}

/*
 * start a scsi operation given the command and the data address.  Also
 * needs the unit, target and lu.
 */
int
uha_scsi_cmd(xs)
	struct scsi_xfer *xs;
{
	struct scsi_link *sc_link = xs->sc_link;
	struct uha_softc *uha = sc_link->adapter_softc;
	struct mscp *mscp;
	struct uha_dma_seg *sg;
	int seg;		/* scatter gather seg being worked on */
	u_long thiskv, thisphys, nextphys;
	int bytes_this_seg, bytes_this_page, datalen, flags;
	struct iovec *iovp;
	int s;
	u_long templen;

	SC_DEBUG(sc_link, SDEV_DB2, ("uha_scsi_cmd\n"));
	/*
	 * get a mscp (mbox-out) to use. If the transfer
	 * is from a buf (possibly from interrupt time)
	 * then we can't allow it to sleep
	 */
	flags = xs->flags;
	if (xs->bp)
		flags |= SCSI_NOSLEEP;	/* just to be sure */
	if (flags & ITSDONE) {
		printf("%s: already done?", uha->sc_dev.dv_xname);
		xs->flags &= ~ITSDONE;
	}
	if (!(flags & INUSE)) {
		printf("%s: not in use?", uha->sc_dev.dv_xname);
		xs->flags |= INUSE;
	}
	if (!(mscp = uha_get_mscp(uha, flags))) {
		xs->error = XS_DRIVER_STUFFUP;
		return TRY_AGAIN_LATER;
	}
	SC_DEBUG(sc_link, SDEV_DB3, ("start mscp(%x)\n", mscp));
	mscp->xs = xs;

	/*
	 * Put all the arguments for the xfer in the mscp
	 */
	if (flags & SCSI_RESET) {
		mscp->opcode = 0x04;
		mscp->ca = 0x01;
	} else {
		mscp->opcode = 0x02;
		mscp->ca = 0x01;
	}
	if (flags & SCSI_DATA_IN)
		mscp->xdir = 0x01;
	if (flags & SCSI_DATA_OUT)
		mscp->xdir = 0x02;

	mscp->dcn = 0x00;
	mscp->chan = 0x00;
	mscp->target = sc_link->target;
	mscp->lun = sc_link->lun;
	mscp->link.addr[0] = 0x00;
	mscp->link.addr[1] = 0x00;
	mscp->link.addr[2] = 0x00;
	mscp->link.addr[3] = 0x00;
	mscp->link_id = 0x00;
	mscp->cdblen = xs->cmdlen;
	scratch = KVTOPHYS(&mscp->mscp_sense);
	mscp->sense.addr[0] = (scratch & 0xff);
	mscp->sense.addr[1] = ((scratch >> 8) & 0xff);
	mscp->sense.addr[2] = ((scratch >> 16) & 0xff);
	mscp->sense.addr[3] = ((scratch >> 24) & 0xff);
	mscp->senselen = sizeof(mscp->mscp_sense);
	mscp->ha_status = 0x00;
	mscp->targ_status = 0x00;

	if (xs->datalen) {	/* should use S/G only if not zero length */
		scratch = KVTOPHYS(mscp->uha_dma);
		mscp->data.addr[0] = (scratch & 0xff);
		mscp->data.addr[1] = ((scratch >> 8) & 0xff);
		mscp->data.addr[2] = ((scratch >> 16) & 0xff);
		mscp->data.addr[3] = ((scratch >> 24) & 0xff);
		sg = mscp->uha_dma;
		seg = 0;
		mscp->sgth = 0x01;

#ifdef	TFS
		if (flags & SCSI_DATA_UIO) {
			iovp = ((struct uio *) xs->data)->uio_iov;
			datalen = ((struct uio *) xs->data)->uio_iovcnt;
			xs->datalen = 0;
			while (datalen && seg < UHA_NSEG) {
				scratch = (u_long) iovp->iov_base;
				sg->addr.addr[0] = (scratch & 0xff);
				sg->addr.addr[1] = ((scratch >> 8) & 0xff);
				sg->addr.addr[2] = ((scratch >> 16) & 0xff);
				sg->addr.addr[3] = ((scratch >> 24) & 0xff);
				xs->datalen += *(u_long *) sg->len.len = iovp->iov_len;
				SC_DEBUGN(sc_link, SDEV_DB4, ("(0x%x@0x%x)",
					iovp->iov_len, iovp->iov_base));
				sg++;
				iovp++;
				seg++;
				datalen--;
			}
		} else
#endif /*TFS */
		{
			/*
			 * Set up the scatter gather block
			 */
			SC_DEBUG(sc_link, SDEV_DB4,
				("%d @0x%x:- ", xs->datalen, xs->data));
			datalen = xs->datalen;
			thiskv = (int) xs->data;
			thisphys = KVTOPHYS(thiskv);
			templen = 0;

			while (datalen && seg < UHA_NSEG) {
				bytes_this_seg = 0;

				/* put in the base address */
				sg->addr.addr[0] = (thisphys & 0xff);
				sg->addr.addr[1] = ((thisphys >> 8) & 0xff);
				sg->addr.addr[2] = ((thisphys >> 16) & 0xff);
				sg->addr.addr[3] = ((thisphys >> 24) & 0xff);

				SC_DEBUGN(sc_link, SDEV_DB4,
					("0x%x", thisphys));

				/* do it at least once */
				nextphys = thisphys;
				while (datalen && thisphys == nextphys) {
					/*
					 * This page is contiguous (physically)
					 * with the the last, just extend the
					 * length 
					 */
					/* how far to the end of the page */
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
				sg->len.len[0] = (bytes_this_seg & 0xff);
				sg->len.len[1] = ((bytes_this_seg >> 8) & 0xff);
				sg->len.len[2] = ((bytes_this_seg >> 16) & 0xff);
				sg->len.len[3] = ((bytes_this_seg >> 24) & 0xff);
				templen += bytes_this_seg;
				sg++;
				seg++;
			}
		}

		/* end of iov/kv decision */
		mscp->datalen.len[0] = (templen & 0xff);
		mscp->datalen.len[1] = ((templen >> 8) & 0xff);
		mscp->datalen.len[2] = ((templen >> 16) & 0xff);
		mscp->datalen.len[3] = ((templen >> 24) & 0xff);
		mscp->sg_num = seg;
		SC_DEBUGN(sc_link, SDEV_DB4, ("\n"));

		if (datalen) {	/* there's still data, must have run out of segs! */
			printf("%s: uha_scsi_cmd, more than %d DMA segs\n",
				uha->sc_dev.dv_xname, UHA_NSEG);
			xs->error = XS_DRIVER_STUFFUP;
			uha_free_mscp(uha, mscp, flags);
			return HAD_ERROR;
		}
	} else {		/* No data xfer, use non S/G values */
		mscp->data.addr[0] = 0x00;
		mscp->data.addr[1] = 0x00;
		mscp->data.addr[2] = 0x00;
		mscp->data.addr[3] = 0x00;
		mscp->datalen.len[0] = 0x00;
		mscp->datalen.len[1] = 0x00;
		mscp->datalen.len[2] = 0x00;
		mscp->datalen.len[3] = 0x00;
		mscp->xdir = 0x03;
		mscp->sgth = 0x00;
		mscp->sg_num = 0x00;
	}

	/*
	 * Put the scsi command in the mscp and start it
	 */
	bcopy(xs->cmd, mscp->cdb, xs->cmdlen);

	/*
	 * Usually return SUCCESSFULLY QUEUED
	 */
	if (!(flags & SCSI_NOMASK)) {
		s = splbio();
		(uha->send_mbox)(uha, mscp);
		timeout(uha_timeout, mscp, (xs->timeout * hz) / 1000);
		splx(s);
		SC_DEBUG(sc_link, SDEV_DB3, ("cmd_sent\n"));
		return SUCCESSFULLY_QUEUED;
	}

	/*
	 * If we can't use interrupts, poll on completion
	 */
	(uha->send_mbox)(uha, mscp);
	SC_DEBUG(sc_link, SDEV_DB3, ("cmd_wait\n"));
	do {
		if ((uha->poll)(uha, xs->timeout)) {
			if (!(xs->flags & SCSI_SILENT))
				printf("%s: cmd fail\n", uha->sc_dev.dv_xname);
			if (!(uha->abort)(uha, mscp)) {
				printf("%s: abort failed in wait\n",
					uha->sc_dev.dv_xname);
				uha_free_mscp(uha, mscp, flags);
			}
			xs->error = XS_DRIVER_STUFFUP;
			return HAD_ERROR;
		}
	} while (!(xs->flags & ITSDONE));/* something (?) else finished */
	if (xs->error)
		return HAD_ERROR;
	return COMPLETE;
}

void
uha_timeout(arg)
	void *arg;
{
	int s = splbio();
	struct mscp *mscp = (struct mscp *)arg;
	struct uha_softc *uha = mscp->xs->sc_link->adapter_softc;

	sc_print_addr(mscp->xs->sc_link);
	printf("timed out");

#ifdef	UHADEBUG
	uha_print_active_mscp(uha);
#endif /*UHADEBUG */

	if (!(uha->abort)(uha, mscp) || (mscp->flags == MSCP_ABORTED)) {
		printf(" AGAIN\n");
		mscp->xs->retries = 0;	/* I MEAN IT ! */
		uha_done(uha, mscp);
	} else {		/* abort the operation that has timed out */
		printf("\n");
		mscp->flags = MSCP_ABORTED;
		timeout(uha_timeout, mscp, 2 * hz);
	}
	splx(s);
}

#ifdef	UHADEBUG
void
uha_print_mscp(mscp)
	struct mscp *mscp;
{

	printf("mscp:%x op:%x cmdlen:%d senlen:%d\n",
		mscp, mscp->opcode, mscp->cdblen, mscp->senselen);
	printf("	sg:%d sgnum:%x datlen:%d hstat:%x tstat:%x flags:%x\n",
		mscp->sgth, mscp->sg_num, mscp->datalen, mscp->ha_status,
		mscp->targ_status, mscp->flags);
	show_scsi_cmd(mscp->xs);
}

void
uha_print_active_mscp(uha)
	struct uha_softc *uha;
{
	struct mscp *mscp;
	int i = 0;

	while (i++ < MSCP_HASH_SIZE) {
		mscp = uha->mscphash[i];
		while (mscp) {
			if (mscp->flags != MSCP_FREE)
				uha_print_mscp(mscp);
			mscp = mscp->nexthash;
		}
	}
}
#endif /*UHADEBUG */
