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
 * commenced: Sun Sep 27 18:14:01 PDT 1992
 *
 *      $Id: aha1742.c,v 1.14.2.3 1993/11/29 00:35:57 mycroft Exp $
 */

#include "ahb.h"

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

typedef u_long physaddr;

#define KVTOPHYS(x)   vtophys(x)

#define AHB_ECB_MAX	32	/* store up to 32ECBs at any one time     */
				/* in aha1742 H/W ( Not MAX ? )         */
#define	ECB_HASH_SIZE	32	/* when we have a physical addr. for      */
				/* a ecb and need to find the ecb in    */
				/* space, look it up in the hash table  */
#define	ECB_HASH_SHIFT	9	/* only hash on multiples of 512  */
#define ECB_HASH(x)	((((long int)(x))>>ECB_HASH_SHIFT) % ECB_HASH_SIZE)

#define	AHB_NSEG	33	/* number of dma segments supported       */

/*
 * AHA1740 standard EISA Host ID regs  (Offset from slot base)
 */
#define HID0		0xC80	/* 0,1: msb of ID2, 3-7: ID1      */
#define HID1		0xC81	/* 0-4: ID3, 4-7: LSB ID2         */
#define HID2		0xC82	/* product, 0=174[20] 1 = 1744    */
#define HID3		0xC83	/* firmware revision              */

#define CHAR1(B1,B2) (((B1>>2) & 0x1F) | '@')
#define CHAR2(B1,B2) (((B1<<3) & 0x18) | ((B2>>5) & 0x7)|'@')
#define CHAR3(B1,B2) ((B2 & 0x1F) | '@')

/*
 * AHA1740 EISA board control registers (Offset from slot base)
 */
#define	EBCTRL		0xC84
#define  CDEN		0x01

/*
 * AHA1740 EISA board mode registers (Offset from slot base)
 */
#define PORTADDR	0xCC0
#define	 PORTADDR_ENHANCED	0x80
#define BIOSADDR	0xCC1
#define	INTDEF		0xCC2
#define	SCSIDEF		0xCC3
#define	BUSDEF		0xCC4
#define	RESV0		0xCC5
#define	RESV1		0xCC6
#define	RESV2		0xCC7
/**** bit definitions for INTDEF ****/
#define	INT9	0x00
#define	INT10	0x01
#define	INT11	0x02
#define	INT12	0x03
#define	INT14	0x05
#define	INT15	0x06
#define INTHIGH 0x08		/* int high=ACTIVE (else edge) */
#define	INTEN	0x10
/**** bit definitions for SCSIDEF ****/
#define	HSCSIID	0x0F		/* our SCSI ID */
#define	RSTPWR	0x10		/* reset scsi bus on power up or reset */
/**** bit definitions for BUSDEF ****/
#define	B0uS	0x00		/* give up bus immediatly */
#define	B4uS	0x01		/* delay 4uSec. */
#define	B8uS	0x02

/*
 * AHA1740 ENHANCED mode mailbox control regs (Offset from slot base)
 */
#define MBOXOUT0	0xCD0
#define MBOXOUT1	0xCD1
#define MBOXOUT2	0xCD2
#define MBOXOUT3	0xCD3

#define	ATTN		0xCD4
#define	G2CNTRL		0xCD5
#define	G2INTST		0xCD6
#define G2STAT		0xCD7

#define	MBOXIN0		0xCD8
#define	MBOXIN1		0xCD9
#define	MBOXIN2		0xCDA
#define	MBOXIN3		0xCDB

#define G2STAT2		0xCDC

/*
 * Bit definitions for the 5 control/status registers
 */
#define	ATTN_TARGET		0x0F
#define	ATTN_OPCODE		0xF0
#define  OP_IMMED		0x10
#define	  AHB_TARG_RESET	0x80
#define  OP_START_ECB		0x40
#define  OP_ABORT_ECB		0x50

#define	G2CNTRL_SET_HOST_READY	0x20
#define	G2CNTRL_CLEAR_EISA_INT	0x40
#define	G2CNTRL_HARD_RESET	0x80

#define	G2INTST_TARGET		0x0F
#define	G2INTST_INT_STAT	0xF0
#define	 AHB_ECB_OK		0x10
#define	 AHB_ECB_RECOVERED	0x50
#define	 AHB_HW_ERR		0x70
#define	 AHB_IMMED_OK		0xA0
#define	 AHB_ECB_ERR		0xC0
#define	 AHB_ASN		0xD0	/* for target mode */
#define	 AHB_IMMED_ERR		0xE0

#define	G2STAT_BUSY		0x01
#define	G2STAT_INT_PEND		0x02
#define	G2STAT_MBOX_EMPTY	0x04

#define	G2STAT2_HOST_READY	0x01

struct ahb_dma_seg {
	physaddr addr;
	long    len;
};

struct ahb_ecb_status {
	u_short status;
#define	ST_DON	0x0001
#define	ST_DU	0x0002
#define	ST_QF	0x0008
#define	ST_SC	0x0010
#define	ST_DO	0x0020
#define	ST_CH	0x0040
#define	ST_INT	0x0080
#define	ST_ASA	0x0100
#define	ST_SNS	0x0200
#define	ST_INI	0x0800
#define	ST_ME	0x1000
#define	ST_ECA	0x4000
	u_char  ha_status;
#define	HS_OK			0x00
#define	HS_CMD_ABORTED_HOST	0x04
#define	HS_CMD_ABORTED_ADAPTER	0x05
#define	HS_TIMED_OUT		0x11
#define	HS_HARDWARE_ERR		0x20
#define	HS_SCSI_RESET_ADAPTER	0x22
#define	HS_SCSI_RESET_INCOMING	0x23
	u_char  targ_status;
#define	TS_OK			0x00
#define	TS_CHECK_CONDITION	0x02
#define	TS_BUSY			0x08
	u_long  resid_count;
	u_long  resid_addr;
	u_short addit_status;
	u_char  sense_len;
	u_char  unused[9];
	u_char  cdb[6];
};

struct ecb {
	u_char  opcode;
#define	ECB_SCSI_OP	0x01
	        u_char:4;
	u_char  options:3;
	        u_char:1;
	short   opt1;
#define	ECB_CNE	0x0001
#define	ECB_DI	0x0080
#define	ECB_SES	0x0400
#define	ECB_S_G	0x1000
#define	ECB_DSB	0x4000
#define	ECB_ARS	0x8000
	short   opt2;
#define	ECB_LUN	0x0007
#define	ECB_TAG	0x0008
#define	ECB_TT	0x0030
#define	ECB_ND	0x0040
#define	ECB_DAT	0x0100
#define	ECB_DIR	0x0200
#define	ECB_ST	0x0400
#define	ECB_CHK	0x0800
#define	ECB_REC	0x4000
#define	ECB_NRB	0x8000
	u_short unused1;
	physaddr data;
	u_long  datalen;
	physaddr status;
	physaddr chain;
	short   unused2;
	short   unused3;
	physaddr sense;
	u_char  senselen;
	u_char  cdblen;
	short   cksum;
	u_char  cdb[12];
	/*-----------------end of hardware supported fields----------------*/
	struct ecb *next;	/* in free list */
	struct scsi_xfer *xs;	/* the scsi_xfer for this cmd */
	int     flags;
#define ECB_FREE	0
#define ECB_ACTIVE	1
#define ECB_ABORTED	2
#define ECB_IMMED	4
#define ECB_IMMED_FAIL	8
	struct ahb_dma_seg ahb_dma[AHB_NSEG];
	struct ahb_ecb_status ecb_status;
	struct scsi_sense_data ecb_sense;
	struct ecb *nexthash;
	physaddr hashkey;	/* physaddr of this struct */
};

struct ahb_data {
	struct device sc_dev;
	struct isadev sc_id;
	struct intrhand sc_ih;

	u_short baseport;
	struct ecb *ecbhash[ECB_HASH_SIZE];
	struct ecb *free_ecb;
	int our_id;		/* our scsi id */
	int vect;
	struct ecb *immed_ecb;	/* an outstanding immediete command */
	int numecbs;
	struct scsi_link sc_link;
}	*ahbdata[NAHB];

void ahb_send_mbox __P((struct ahb_data *, int, int, struct ecb *));
int ahb_poll __P((struct ahb_data *, int));
void ahb_send_immed __P((struct ahb_data *, int, u_long));
int ahbprobe __P((struct device *, struct cfdata *, void *));
int ahbprobe1 __P((struct device *, struct cfdata *, struct isa_attach_args *));
void ahbattach __P((struct device *, struct device *, void *));
u_int ahb_adapter_info __P((struct ahb_data *));
int ahbintr __P((void *));
void ahb_done __P((struct ahb_data *, struct ecb *, int));
void ahb_free_ecb __P((struct ahb_data *, struct ecb *, int));
struct ecb *ahb_get_ecb __P((struct ahb_data *, int));
struct ecb *ahb_ecb_phys_kv __P((struct ahb_data *, physaddr));
int ahb_find __P((struct ahb_data *));
void ahb_init __P((struct ahb_data *));
void ahbminphys __P((struct buf *));
int ahb_scsi_cmd __P((struct scsi_xfer *));
void ahb_timeout __P((caddr_t));
void ahb_print_ecb __P((struct ecb *));
void ahb_print_active_ecb __P((struct ahb_data *));

struct	ecb *cheat;

#define	MAX_SLOTS	8
static  ahb_slot = 0;		/* slot last board was found in */
int     ahb_debug = 0;
#define AHB_SHOWECBS 0x01
#define AHB_SHOWINTS 0x02
#define AHB_SHOWCMDS 0x04
#define AHB_SHOWMISC 0x08

struct cfdriver ahbcd = {
	NULL,
	"ahb",
	ahbprobe,
	ahbattach,
	DV_DULL,
	sizeof(struct ahb_data)
};

struct scsi_adapter ahb_switch =
{
	ahb_scsi_cmd,
	ahbminphys,
	0,
	0,
	ahb_adapter_info,
	"ahb"
};

/* the below structure is so we have a default dev struct for our link struct */
struct scsi_device ahb_dev =
{
	NULL,			/* Use default error handler */
	NULL,			/* have a queue, served by this */
	NULL,			/* have no async handler */
	NULL,			/* Use default 'done' routine */
	"ahb",
	0
};

/*
 * Function to send a command out through a mailbox
 */
void
ahb_send_mbox(ahb, opcode, target, ecb)
	struct ahb_data *ahb;
	int opcode, target;
	struct ecb *ecb;
{
	u_short port = ahb->baseport;
	u_short stport = port + G2STAT;
	int wait = 100;	/* 1ms should be enough */
	int s = splbio();

	while (--wait) {
		if ((inb(stport) & (G2STAT_BUSY | G2STAT_MBOX_EMPTY))
		    == (G2STAT_MBOX_EMPTY))
			break;
		delay(10);
	}
	if (!wait) {
		printf("%s: board not responding\n", ahb->sc_dev.dv_xname);
		Debugger();
	}

	outl(port + MBOXOUT0, KVTOPHYS(ecb));	/* don't know this will work */
	outb(port + ATTN, opcode | target);

	splx(s);
}

/*
 * Function to poll for command completion when in poll mode
 */
int
ahb_poll(ahb, wait)
	struct ahb_data *ahb;
	int wait;
{				/* in msec  */
	u_short port = ahb->baseport;
	u_short stport = port + G2STAT;

    retry:
	while (--wait) {
		if (inb(stport) & G2STAT_INT_PEND)
			break;
		delay(1000);
	}
	if (!wait) {
		printf("%s: board not responding\n", ahb->sc_dev.dv_xname);
		return EIO;
	}

	if (cheat != ahb_ecb_phys_kv(ahb, inl(port + MBOXIN0))) {
		printf("discarding %x ", inl(port + MBOXIN0));
		outb(port + G2CNTRL, G2CNTRL_CLEAR_EISA_INT);
		delay(50000);
		goto retry;
	}

	/* don't know this will work */
	ahbintr(ahb);
	return 0;
}

/*
 * Function to  send an immediate type command to the adapter
 */
void
ahb_send_immed(ahb, target, cmd)
	struct ahb_data *ahb;
	int target;
	u_long cmd;
{
	u_short port = ahb->baseport;
	u_short stport = port + G2STAT;
	int wait = 100;	/* 1 ms enough? */
	int s = splbio();

	while (--wait) {
		if ((inb(stport) & (G2STAT_BUSY | G2STAT_MBOX_EMPTY))
		    == (G2STAT_MBOX_EMPTY))
			break;
		delay(10);
	}
	if (!wait) {
		printf("%s: board not responding\n", ahb->sc_dev.dv_xname);
		Debugger();
	}

	outl(port + MBOXOUT0, cmd);	/* don't know this will work */
	outb(port + G2CNTRL, G2CNTRL_SET_HOST_READY);
	outb(port + ATTN, OP_IMMED | target);
	splx(s);
}

/*
 * Check the slots looking for a board we recognise
 * If we find one, note it's address (slot) and call
 * the actual probe routine to check it out.
 */
int
ahbprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct isa_attach_args *ia = aux;
	u_short port;
	u_char byte1, byte2, byte3;

	if (ia->ia_iobase != IOBASEUNK)
		return ahbprobe1(parent, cf, aux);

	do {
		ahb_slot++;
		port = 0x1000 * ahb_slot;
		byte1 = inb(port + HID0);
		byte2 = inb(port + HID1);
		byte3 = inb(port + HID2);
		if (byte1 == 0xff)
			continue;
		if ((CHAR1(byte1, byte2) != 'A')
		    || (CHAR2(byte1, byte2) != 'D')
		    || (CHAR3(byte1, byte2) != 'P')
		    || ((byte3 != 0) && (byte3 != 1))) {
			continue;
		}
		ia->ia_iobase = port;
		if (ahbprobe1(parent, cf, ia))
			return 1;
	} while (ahb_slot < MAX_SLOTS);

	return 0;
}

/*
 * Check if the device can be found at the port given
 * and if so, set it up ready for further work
 * as an argument, takes the isa_device structure from
 * autoconf.c.
 */
int
ahbprobe1(parent, cf, ia)
	struct device *parent;
	struct cfdata *cf;
	struct isa_attach_args *ia;
{
	int unit = cf->cf_unit;
	struct	ahb_data *ahb;

	ahb = malloc(sizeof(struct ahb_data), M_TEMP, M_NOWAIT);
	if (!ahb) {
		printf("ahb%d: cannot malloc!\n", unit);
		return 0;
	}
	bzero(ahb, sizeof(*ahb));
	ahbdata[unit] = ahb;
	ahb->baseport = ia->ia_iobase;

	/*
	 * Try initialise a unit at this location
	 * sets up dma and bus speed, loads ahb->vect
	 */
	if (ahb_find(ahb) != 0) {
		ahbdata[unit] = NULL;
		free(ahb, M_TEMP);
		return 0;
	}

	if (ia->ia_irq == IRQUNK) {
		ia->ia_irq = (1 << ahb->vect);
	} else {
		if (ia->ia_irq != (1 << ahb->vect)) {
			printf("ahb%d: irq mismatch, %x != %x\n", unit,
				ia->ia_irq, (1 << ahb->vect));
			ahbdata[unit] = NULL;
			free(ahb, M_TEMP);
			return 0;
		}
	}

	ia->ia_drq = DRQUNK;
	ia->ia_msize = 0;
	ia->ia_iosize = 0x1000;
	return 1;
}

ahbprint()
{

}

/*
 * Attach all the sub-devices we can find
 */
void
ahbattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	int unit = self->dv_unit;
	struct ahb_data *ahb = ahbdata[unit];
	struct isa_attach_args *ia = aux;

	bcopy((char *)ahb + sizeof(struct device),
	      (char *)self + sizeof(struct device),
	      sizeof(struct ahb_data) - sizeof(struct device));
	free(ahb, M_TEMP);
	ahbdata[unit] = ahb = (struct ahb_data *)self;

	ahb_init(ahb);

	/*
	 * fill in the prototype scsi_link.
	 */
	ahb->sc_link.adapter_softc = ahb;
	ahb->sc_link.adapter_targ = ahb->our_id;
	ahb->sc_link.adapter = &ahb_switch;
	ahb->sc_link.device = &ahb_dev;

	printf("\n");

	isa_establish(&ahb->sc_id, &ahb->sc_dev);
	ahb->sc_ih.ih_fun = ahbintr;
	ahb->sc_ih.ih_arg = ahb;
	/* XXX and DV_TAPE, but either gives us splbio */
	intr_establish(ia->ia_irq, &ahb->sc_ih, DV_DISK);

	/*
	 * ask the adapter what subunits are present
	 */
	config_found(self, &ahb->sc_link, ahbprint);
}

/*
 * Return some information to the caller about
 * the adapter and it's capabilities
 */
u_int 
ahb_adapter_info(ahb)
	struct ahb_data *ahb;
{

	return 2;	/* 2 outstanding requests at a time per device */
}

/*
 * Catch an interrupt from the adaptor
 */
int
ahbintr(arg)
	void *arg;
{
	struct ahb_data *ahb = arg;
	struct ecb *ecb;
	u_char stat, ahbstat;
	u_long mboxval;
	u_short port = ahb->baseport;

#ifdef	AHBDEBUG
	printf("ahbintr ");
#endif /*AHBDEBUG */

	while (inb(port + G2STAT) & G2STAT_INT_PEND) {
		/*
		 * First get all the information and then 
		 * acknowlege the interrupt
		 */
		ahbstat = inb(port + G2INTST);
		stat = ahbstat & G2INTST_INT_STAT;
		mboxval = inl(port + MBOXIN0);	/* don't know this will work */
		outb(port + G2CNTRL, G2CNTRL_CLEAR_EISA_INT);

#ifdef	AHBDEBUG
		printf("status = 0x%x ", ahbstat);
#endif /*AHBDEBUG */

		/*
		 * Process the completed operation
		 */
		if (stat == AHB_ECB_OK) {	/* common case is fast */
			ecb = ahb_ecb_phys_kv(ahb, mboxval);
		} else {
			switch (stat) {
			case AHB_IMMED_OK:
				ecb = ahb->immed_ecb;
				ahb->immed_ecb = 0;
				break;
			case AHB_IMMED_ERR:
				ecb = ahb->immed_ecb;
				ecb->flags |= ECB_IMMED_FAIL;
				ahb->immed_ecb = 0;
				break;
			case AHB_ASN:	/* for target mode */
				printf("%s: Unexpected ASN interrupt(%x)\n",
					ahb->sc_dev.dv_xname, mboxval);
				ecb = 0;
				break;
			case AHB_HW_ERR:
				printf("%s: Hardware error interrupt(%x)\n",
					ahb->sc_dev.dv_xname, mboxval);
				ecb = 0;
				break;
			case AHB_ECB_RECOVERED:
				ecb = ahb_ecb_phys_kv(ahb, mboxval);
				break;
			case AHB_ECB_ERR:
				ecb = ahb_ecb_phys_kv(ahb, mboxval);
				break;
			default:
				printf("%s: Unknown return %x\n",
					ahb->sc_dev.dv_xname, ahbstat);
				ecb = 0;
			}
		} if (ecb) {
#ifdef	AHBDEBUG
			if (ahb_debug & AHB_SHOWCMDS)
				show_scsi_cmd(ecb->xs);
			if ((ahb_debug & AHB_SHOWECBS) && ecb)
				printf("<int ecb(%x)>", ecb);
#endif /*AHBDEBUG */
			untimeout((timeout_t)ahb_timeout, (caddr_t)ecb);
			ahb_done(ahb, ecb, stat != AHB_ECB_OK);
		}
	}
	return 1;
}

/*
 * We have a ecb which has been processed by the adaptor, now we look to see
 * how the operation went.
 */
void
ahb_done(ahb, ecb, failed)
	struct ahb_data *ahb;
	struct ecb *ecb;
	int failed;
{
	struct ahb_ecb_status *stat = &ecb->ecb_status;
	struct scsi_sense_data *s1, *s2;
	struct scsi_xfer *xs = ecb->xs;

	SC_DEBUG(xs->sc_link, SDEV_DB2, ("ahb_done\n"));
	/*
	 * Otherwise, put the results of the operation
	 * into the xfer and call whoever started it
	 */
	if (ecb->flags & ECB_IMMED) {
		if (ecb->flags & ECB_IMMED_FAIL)
			xs->error = XS_DRIVER_STUFFUP;
		goto done;
	}

	if (!failed || (xs->flags & SCSI_ERR_OK)) {	/* All went correctly  OR errors expected */
		xs->resid = 0;
		xs->error = 0;
	} else {
		s1 = &(ecb->ecb_sense);
		s2 = &(xs->sense);

		if (stat->ha_status) {
			switch (stat->ha_status) {
			case HS_SCSI_RESET_ADAPTER:
				break;
			case HS_SCSI_RESET_INCOMING:
				break;
			case HS_CMD_ABORTED_HOST:	/* No response */
			case HS_CMD_ABORTED_ADAPTER:	/* No response */
				break;
			case HS_TIMED_OUT:	/* No response */
#ifdef	AHBDEBUG
				if (ahb_debug & AHB_SHOWMISC)
					printf("timeout reported back\n");
#endif /*AHBDEBUG */
				xs->error = XS_TIMEOUT;
				break;
			default:	/* Other scsi protocol messes */
				xs->error = XS_DRIVER_STUFFUP;
#ifdef	AHBDEBUG
				if (ahb_debug & AHB_SHOWMISC)
					printf("unexpected ha_status %x\n",
						stat->ha_status);
#endif /*AHBDEBUG */ 
			}
		} else {
			switch (stat->targ_status) {
			case TS_CHECK_CONDITION:
				/* structure copy!!!!! */
				*s2 = *s1;
				xs->error = XS_SENSE;
				break;
			case TS_BUSY:
				xs->error = XS_BUSY;
				break;
			default:
#ifdef	AHBDEBUG
				if (ahb_debug & AHB_SHOWMISC) {
					printf("unexpected targ_status %x\n",
						stat->targ_status);
				}
#endif /*AHBDEBUG */
				xs->error = XS_DRIVER_STUFFUP;
			}
		}
	}
    done:
	xs->flags |= ITSDONE;
	ahb_free_ecb(ahb, ecb, xs->flags);
	scsi_done(xs);
}

/*
 * A ecb (and hence a mbx-out is put onto the 
 * free list.
 */
void
ahb_free_ecb(ahb, ecb, flags)
	struct ahb_data *ahb;
	struct ecb *ecb;
	int flags;
{
	int opri;

	if (!(flags & SCSI_NOMASK))
		opri = splbio();

	ecb->next = ahb->free_ecb;
	ahb->free_ecb = ecb;
	ecb->flags = ECB_FREE;
	/*
	 * If there were none, wake abybody waiting for
	 * one to come free, starting with queued entries
	 */
	if (!ecb->next)
		wakeup((caddr_t)&ahb->free_ecb);

	if (!(flags & SCSI_NOMASK))
		splx(opri);
}

/*
 * Get a free ecb 
 *
 * If there are none, see if we can allocate a new one. If so, put it in the
 * hash table too otherwise either return an error or sleep.
 */
struct ecb *
ahb_get_ecb(ahb, flags)
	struct ahb_data *ahb;
	int flags;
{
	int opri;
	struct ecb *ecbp;
	int hashnum;

	if (!(flags & SCSI_NOMASK))
		opri = splbio();

	/*
	 * If we can and have to, sleep waiting for one to come free
	 * but only if we can't allocate a new one.
	 */
	while (!(ecbp = ahb->free_ecb)) {
		if (ahb->numecbs < AHB_ECB_MAX) {
			if (ecbp = (struct ecb *) malloc(sizeof(struct ecb),
			    M_TEMP,
			    M_NOWAIT)) {
				bzero(ecbp, sizeof(struct ecb));
				ahb->numecbs++;
				ecbp->flags = ECB_ACTIVE;
				/*
				 * put in the phystokv hash table
				 * Never gets taken out.
				 */
				ecbp->hashkey = KVTOPHYS(ecbp);
				hashnum = ECB_HASH(ecbp->hashkey);
				ecbp->nexthash = ahb->ecbhash[hashnum];
				ahb->ecbhash[hashnum] = ecbp;
			} else {
				printf("%s: Can't malloc ECB\n",
					ahb->sc_dev.dv_xname);
			}
			goto gottit;
		} else {
			if (!(flags & SCSI_NOSLEEP))
				tsleep((caddr_t)&ahb->free_ecb, PRIBIO,
				    "ahbecb", 0);
		}
	}
	if (ecbp) {
		/* Get ECB from from free list */
		ahb->free_ecb = ecbp->next;
		ecbp->flags = ECB_ACTIVE;
	}
    gottit:
	if (!(flags & SCSI_NOMASK))
		splx(opri);

	return ecbp;
}

/*
 * given a physical address, find the ecb that it corresponds to.
 */
struct ecb *
ahb_ecb_phys_kv(ahb, ecb_phys)
	struct ahb_data *ahb;
	physaddr ecb_phys;
{
	int hashnum = ECB_HASH(ecb_phys);
	struct ecb *ecbp = ahb->ecbhash[hashnum];

	while (ecbp) {
		if (ecbp->hashkey == ecb_phys)
			break;
		ecbp = ecbp->nexthash;
	}
	return ecbp;
}

/*
 * Start the board, ready for normal operation
 */
int
ahb_find(ahb)
	struct ahb_data *ahb;
{
	u_short port = ahb->baseport;
	u_short stport = port + G2STAT;
	u_char intdef;
	int i;
	int wait = 1000;	/* 1 sec enough? */

#define	NO_NO 1
#ifdef NO_NO
	/*
	 * reset board, If it doesn't respond, assume 
	 * that it's not there.. good for the probe
	 */
	outb(port + EBCTRL, CDEN);	/* enable full card */
	outb(port + PORTADDR, PORTADDR_ENHANCED);

	outb(port + G2CNTRL, G2CNTRL_HARD_RESET);
	delay(1000);
	outb(port + G2CNTRL, 0);
	delay(10000);
	while (--wait) {
		if ((inb(stport) & G2STAT_BUSY) == 0)
			break;
		delay(1000);
	}
	if (!wait) {
#ifdef	AHBDEBUG
		if (ahb_debug & AHB_SHOWMISC)
			printf("ahb_find: No answer from aha1742 board\n");
#endif /*AHBDEBUG */
		return ENXIO;
	}
	i = inb(port + MBOXIN0);
	if (i) {
		printf("self test failed, val = 0x%x\n", i);
		return EIO;
	}
#endif

	while (inb(stport) & G2STAT_INT_PEND) {
		printf(".");
		outb(port + G2CNTRL, G2CNTRL_CLEAR_EISA_INT);
		delay(10000);
	}
	outb(port + EBCTRL, CDEN);	/* enable full card */
	outb(port + PORTADDR, PORTADDR_ENHANCED);

	intdef = inb(port + INTDEF);
	switch (intdef & 0x07) {
	case INT9:
		ahb->vect = 9;
		break;
	case INT10:
		ahb->vect = 10;
		break;
	case INT11:
		ahb->vect = 11;
		break;
	case INT12:
		ahb->vect = 12;
		break;
	case INT14:
		ahb->vect = 14;
		break;
	case INT15:
		ahb->vect = 15;
		break;
	default:
		printf("illegal int setting %x\n", intdef);
		return EIO;
	}

	outb(port + INTDEF, (intdef | INTEN));	/* make sure we can interrupt */

	/* who are we on the scsi bus? */
	ahb->our_id = (inb(port + SCSIDEF) & HSCSIID);

	/*
	 * Note that we are going and return (to probe)
	 */
	return 0;
}

void
ahb_init(ahb)
	struct ahb_data *ahb;
{

}

void
ahbminphys(bp)
	struct buf *bp;
{

	if (bp->b_bcount > ((AHB_NSEG - 1) << PGSHIFT))
		bp->b_bcount = ((AHB_NSEG - 1) << PGSHIFT);
}

/*
 * start a scsi operation given the command and the data address.  Also needs
 * the unit, target and lu.
 */
int 
ahb_scsi_cmd(xs)
	struct scsi_xfer *xs;
{
	struct scsi_link *sc_link = xs->sc_link;
	struct ahb_data *ahb = sc_link->adapter_softc;
	struct ecb *ecb;
	struct ahb_dma_seg *sg;
	int seg;		/* scatter gather seg being worked on */
	int thiskv;
	physaddr thisphys, nextphys;
	int bytes_this_seg, bytes_this_page, datalen, flags;
	int s;

	SC_DEBUG(sc_link, SDEV_DB2, ("ahb_scsi_cmd\n"));
	/*
	 * get a ecb (mbox-out) to use. If the transfer
	 * is from a buf (possibly from interrupt time)
	 * then we can't allow it to sleep
	 */
	flags = xs->flags;
	if (xs->bp)
		flags |= SCSI_NOSLEEP;	/* just to be sure */
	if (flags & ITSDONE) {
		printf("%s: already done?", ahb->sc_dev.dv_xname);
		xs->flags &= ~ITSDONE;
	}
	if (!(flags & INUSE)) {
		printf("%s: not in use?", ahb->sc_dev.dv_xname);
		xs->flags |= INUSE;
	}
	if (!(ecb = ahb_get_ecb(ahb, flags))) {
		xs->error = XS_DRIVER_STUFFUP;
		return TRY_AGAIN_LATER;
	}
	cheat = ecb;
	SC_DEBUG(sc_link, SDEV_DB3, ("start ecb(%x)\n", ecb));
	ecb->xs = xs;

	/*
	 * If it's a reset, we need to do an 'immediate'
	 * command, and store it's ecb for later
	 * if there is already an immediate waiting, 
	 * then WE must wait
	 */
	if (flags & SCSI_RESET) {
		ecb->flags |= ECB_IMMED;
		if (ahb->immed_ecb)
			return TRY_AGAIN_LATER;
		ahb->immed_ecb = ecb;
		if (!(flags & SCSI_NOMASK)) {
			s = splbio();
			ahb_send_immed(ahb, sc_link->target, AHB_TARG_RESET);
			timeout(ahb_timeout, (caddr_t)ecb, (xs->timeout * hz) / 1000);
			splx(s);
			return SUCCESSFULLY_QUEUED;
		} else {
			ahb_send_immed(ahb, sc_link->target, AHB_TARG_RESET);
			/*
			 * If we can't use interrupts, poll on completion
			 */
			SC_DEBUG(sc_link, SDEV_DB3, ("wait\n"));
			if (ahb_poll(ahb, xs->timeout)) {
				ahb_free_ecb(ahb, ecb, flags);
				xs->error = XS_TIMEOUT;
				return HAD_ERROR;
			}
			return COMPLETE;
		}
	}
	/*
	 * Put all the arguments for the xfer in the ecb
	 */
	ecb->opcode = ECB_SCSI_OP;
	ecb->opt1 = ECB_SES | ECB_DSB | ECB_ARS;
	if (xs->datalen)
		ecb->opt1 |= ECB_S_G;
	ecb->opt2 = sc_link->lun | ECB_NRB;
	ecb->cdblen = xs->cmdlen;
	ecb->sense = KVTOPHYS(&ecb->ecb_sense);
	ecb->senselen = sizeof(ecb->ecb_sense);
	ecb->status = KVTOPHYS(&ecb->ecb_status);

	if (xs->datalen) {	/* should use S/G only if not zero length */
		ecb->data = KVTOPHYS(ecb->ahb_dma);
		sg = ecb->ahb_dma;
		seg = 0;

#ifdef	TFS
		if (flags & SCSI_DATA_UIO) {
			iovp = ((struct uio *) xs->data)->uio_iov;
			datalen = ((struct uio *) xs->data)->uio_iovcnt;
			xs->datalen = 0;
			while (datalen && seg < AHB_NSEG) {
				sg->addr = (physaddr) iovp->iov_base;
				xs->datalen += sg->len = iovp->iov_len;
				SC_DEBUGN(sc_link, SDEV_DB4, ("(0x%x@0x%x)",
					iovp->iov_len, iovp->iov_base));
				sg++;
				iovp++;
				seg++;
				datalen--;
			}
		}
		else
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

			while (datalen && seg < AHB_NSEG) {
				bytes_this_seg = 0;

				/* put in the base address */
				sg->addr = thisphys;

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
				sg->len = bytes_this_seg;
				sg++;
				seg++;
			}
		}

		/*end of iov/kv decision */
		ecb->datalen = seg * sizeof(struct ahb_dma_seg);
		SC_DEBUGN(sc_link, SDEV_DB4, ("\n"));

		if (datalen) {
			/*
			 * there's still data, must have run out of segs!
			 */
			printf("%s: ahb_scsi_cmd, more than %d DMA segs\n",
				ahb->sc_dev.dv_xname, AHB_NSEG);
			xs->error = XS_DRIVER_STUFFUP;
			ahb_free_ecb(ahb, ecb, flags);
			return HAD_ERROR;
		}
	} else {	/* No data xfer, use non S/G values */
		ecb->data = (physaddr) 0;
		ecb->datalen = 0;
	}
	ecb->chain = (physaddr) 0;

	/*
	 * Put the scsi command in the ecb and start it
	 */
	bcopy(xs->cmd, ecb->cdb, xs->cmdlen);

	/*
	 * Usually return SUCCESSFULLY QUEUED
	 */
	if (!(flags & SCSI_NOMASK)) {
		s = splbio();
		ahb_send_mbox(ahb, OP_START_ECB, sc_link->target, ecb);
		timeout(ahb_timeout, (caddr_t)ecb, (xs->timeout * hz) / 1000);
		splx(s);
		SC_DEBUG(sc_link, SDEV_DB3, ("cmd_sent\n"));
		return SUCCESSFULLY_QUEUED;
	}

	/*
	 * If we can't use interrupts, poll on completion
	 */
	ahb_send_mbox(ahb, OP_START_ECB, sc_link->target, ecb);
	SC_DEBUG(sc_link, SDEV_DB3, ("cmd_wait\n"));
	do {
		if (ahb_poll(ahb, xs->timeout)) {
			if (!(xs->flags & SCSI_SILENT))
				printf("%s: cmd fail\n", ahb->sc_dev.dv_xname);
			ahb_send_mbox(ahb, OP_ABORT_ECB, sc_link->target, ecb);
			if (ahb_poll(ahb, 2000)) {
				printf("%s: abort failed in wait\n",
					ahb->sc_dev.dv_xname);
				ahb_free_ecb(ahb, ecb, flags);
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
ahb_timeout(arg)
	caddr_t arg;
{
	int s = splbio();
	struct ecb *ecb = (void *)arg;
	struct ahb_data *ahb = ecb->xs->sc_link->adapter_softc;

	sc_print_addr(ecb->xs->sc_link);
	printf("timed out ");

#ifdef	AHBDEBUG
	if (ahb_debug & AHB_SHOWECBS)
		ahb_print_active_ecb(ahb);
#endif /*AHBDEBUG */

	/*
	 * If it's immediate, don't try abort it 
	 */
	if (ecb->flags & ECB_IMMED) {
		ecb->xs->retries = 0;	/* I MEAN IT ! */
		ecb->flags |= ECB_IMMED_FAIL;
		ahb_done(ahb, ecb, 1);
		splx(s);
		return;
	}

	/*
	 * If it has been through before, then
	 * a previous abort has failed, don't
	 * try abort again
	 */
	if (ecb->flags == ECB_ABORTED) {
		printf("AGAIN\n");
		ecb->xs->retries = 0;	/* I MEAN IT ! */
		ecb->ecb_status.ha_status = HS_CMD_ABORTED_HOST;
		ahb_done(ahb, ecb, 1);
	} else {		/* abort the operation that has timed out */
		printf("\n");
		ahb_send_mbox(ahb, OP_ABORT_ECB, ecb->xs->sc_link->target, ecb);
		timeout(ahb_timeout, (caddr_t)ecb, 2 * hz);
		ecb->flags = ECB_ABORTED;
	}
	splx(s);
}

#ifdef	AHBDEBUG
void
ahb_print_ecb(ecb)
	struct ecb *ecb;
{
	printf("ecb:%x op:%x cmdlen:%d senlen:%d\n",
		ecb, ecb->opcode, ecb->cdblen, ecb->senselen);
	printf("	datlen:%d hstat:%x tstat:%x flags:%x\n",
		ecb->datalen, ecb->ecb_status.ha_status,
		ecb->ecb_status.targ_status, ecb->flags);
	show_scsi_cmd(ecb->xs);
}

void
ahb_print_active_ecb(ahb)
	struct ahb_data *ahb;
{
	struct ecb *ecb;
	int i = 0;

	while (i++ < ECB_HASH_SIZE) {
		ecb = ahb->ecbhash[i];
		while (ecb) {
			if (ecb->flags != ECB_FREE)
				ahb_print_ecb(ecb);
			ecb = ecb->nexthash;
		}
	}
}
#endif /*AHBDEBUG */
