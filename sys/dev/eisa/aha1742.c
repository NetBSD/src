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
 *	$Id: aha1742.c,v 1.16 1993/12/20 23:27:33 davidb Exp $
 */

#include "ahb.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/dkbad.h>
#include <sys/disklabel.h>

#include <machine/pio.h>

#include <scsi/scsi_all.h>
#include <scsi/scsiconf.h>

#include <i386/isa/isa_device.h>


#ifdef	DDB
int	Debugger();
#else	DDB
#define	Debugger() panic("should call debugger here (adaptec.c)")
#endif	DDB

typedef unsigned long int physaddr;

#define PHYSTOKV(x)   (x | 0xFE000000)
#define KVTOPHYS(x)   vtophys(x)

extern int delaycount;  /* from clock setup code */
#define	NUM_CONCURRENT	16	/* number of concurrent ops per board */
#define	AHB_NSEG	33	/* number of dma segments supported	 */
#define FUDGE(X)	(X>>1)	/* our loops are slower than spinwait() */


/*
 * AHA1740 standard EISA Host ID regs  (Offset from slot base)
 */
#define HID0		0xC80 /* 0,1: msb of ID2, 3-7: ID1	 */
#define HID1		0xC81 /* 0-4: ID3, 4-7: LSB ID2		 */
#define HID2		0xC82 /* product, 0=174[20] 1 = 1744	 */
#define HID3		0xC83 /* firmware revision		 */

#define CHAR1(B1,B2) (((B1>>2) & 0x1F) | '@')
#define CHAR2(B1,B2) (((B1<<3) & 0x18) | ((B2>>5) & 0x7)|'@')
#define CHAR3(B1,B2) ((B2 & 0x1F) | '@')

/* AHA1740 EISA board control registers (Offset from slot base) */
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

/* bit definitions for INTDEF */
#define	INT9	0x00
#define	INT10	0x01
#define	INT11	0x02
#define	INT12	0x03
#define	INT14	0x05
#define	INT15	0x06
#define INTHIGH 0x08    /* int high=ACTIVE (else edge) */
#define	INTEN	0x10

/* bit definitions for SCSIDEF */
#define	HSCSIID	0x0F	/* our SCSI ID */
#define	RSTPWR	0x10	/* reset scsi bus on power up or reset */

/* bit definitions for BUSDEF */
#define	B0uS	0x00	/* give up bus immediatly */
#define	B4uS	0x01	/* delay 4uSec. */
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
	physaddr	addr;
	long		len;
};

struct ahb_ecb_status {
	u_short	status;
#	 define	ST_DON	0x0001
#	 define	ST_DU	0x0002
#	 define	ST_QF	0x0008
#	 define	ST_SC	0x0010
#	 define	ST_DO	0x0020
#	 define	ST_CH	0x0040
#	 define	ST_INT	0x0080
#	 define	ST_ASA	0x0100
#	 define	ST_SNS	0x0200
#	 define	ST_INI	0x0800
#	 define	ST_ME	0x1000
#	 define	ST_ECA	0x4000
	u_char	ha_status;
#	 define	HS_OK			0x00
#	 define	HS_CMD_ABORTED_HOST	0x04
#	 define	HS_CMD_ABORTED_ADAPTER	0x05
#	 define	HS_TIMED_OUT		0x11
#	 define	HS_HARDWARE_ERR		0x20
#	 define	HS_SCSI_RESET_ADAPTER	0x22
#	 define	HS_SCSI_RESET_INCOMING	0x23
	u_char	targ_status;
#	 define	TS_OK			0x00
#	 define	TS_CHECK_CONDITION	0x02
#	 define	TS_BUSY			0x08
	u_long	resid_count;
	u_long	resid_addr;
	u_short	addit_status;
	u_char	sense_len;
	u_char	unused[9];
	u_char	cdb[6];
};


struct ecb {
	u_char	opcode;
#	 define	ECB_SCSI_OP	0x01
	u_char	:4;
	u_char	options:3;
	u_char	:1;
	short opt1;
#	 define	ECB_CNE	0x0001
#	 define	ECB_DI	0x0080
#	 define	ECB_SES	0x0400
#	 define	ECB_S_G	0x1000
#	 define	ECB_DSB	0x4000
#	 define	ECB_ARS	0x8000
	short opt2;
#	 define	ECB_LUN	0x0007
#	 define	ECB_TAG	0x0008
#	 define	ECB_TT	0x0030
#	 define	ECB_ND	0x0040
#	 define	ECB_DAT	0x0100
#	 define	ECB_DIR	0x0200
#	 define	ECB_ST	0x0400
#	 define	ECB_CHK	0x0800
#	 define	ECB_REC	0x4000
#	 define	ECB_NRB	0x8000
	u_short		unused1;
	physaddr	data;
	u_long		datalen;
	physaddr	status;
	physaddr	chain;
	short		unused2;
	short		unused3;
	physaddr	sense;
	u_char		senselen;
	u_char		cdblen;
	short		cksum;
	u_char		cdb[12];
	/*-----------------end of hardware supported fields----------------*/
	struct ecb	 *next;	/* in free list */
	struct scsi_xfer *xs; /* the scsi_xfer for this cmd */
	long	int	delta;  /* difference from previous*/
	struct ecb	 *later,*sooner;
	int		flags;
#define ECB_FREE	0
#define ECB_ACTIVE	1
#define ECB_ABORTED	2
#define ECB_IMMED	4
#define ECB_IMMED_FAIL	8
	struct ahb_dma_seg	ahb_dma[AHB_NSEG];
	struct ahb_ecb_status	ecb_status;
	struct scsi_sense_data	ecb_sense;
};

struct ecb *ahb_soonest	= (struct ecb *)0;
struct ecb *ahb_latest	= (struct ecb *)0;
long int ahb_furtherest	= 0; /* longest time in the timeout queue */

struct ahb_data {
	int	flags;
#define	AHB_INIT	0x01;
	int	baseport;
	struct ecb ecbs[NUM_CONCURRENT];
	struct ecb *free_ecb;
	int	our_id;			/* our scsi id */
	int	vect;
	struct ecb *immed_ecb; 	/* an outstanding immediete command */
} ahb_data[NAHB];

struct ecb *cheat;

#define	MAX_SLOTS	8
static	ahb_slot = 0;	/* slot last board was found in */
static	ahb_unit = 0;
int	ahb_debug = 0;
#define AHB_SHOWECBS 0x01
#define AHB_SHOWINTS 0x02
#define AHB_SHOWCMDS 0x04
#define AHB_SHOWMISC 0x08
#define FAIL	1
#define SUCCESS 0
#define PAGESIZ 4096


int ahbprobe(struct isa_device *);
int ahbprobe1(struct isa_device *);
int ahb_attach(struct isa_device *);
long int ahb_adapter_info(int);
int ahbintr(int);
void ahb_done(int, struct ecb *, int);
void ahb_free_ecb(int, struct ecb *, int);
struct ecb * ahb_get_ecb(int, int);
int ahb_init(int);
void ahbminphys(struct buf *);
int ahb_scsi_cmd(struct scsi_xfer *);
void ahb_add_timeout(struct ecb *, int);
void ahb_remove_timeout(struct ecb *);
void ahb_timeout(int);
void ahb_show_scsi_cmd(struct scsi_xfer *);
void ahb_print_ecb(struct ecb *);
void ahb_print_active_ecb(void);


struct isa_driver ahbdriver = {
	ahbprobe,
	ahb_attach,
	"ahb"
};

struct scsi_switch ahb_switch[NAHB];

/*
 * Function to send a command out through a mailbox
 */
void
ahb_send_mbox(int unit, int opcode, int target, struct ecb *ecb)
{
	int port = ahb_data[unit].baseport;
	int spincount = FUDGE(delaycount) * 1;	/* 1ms should be enough */
	int stport = port + G2STAT, s;

	s = splbio();
	while( ((inb(stport) &
	    (G2STAT_BUSY | G2STAT_MBOX_EMPTY)) != G2STAT_MBOX_EMPTY)
	    && spincount--)
		;
	if(spincount == -1) {
		printf("ahb%d: board not responding\n",unit);
		Debugger();
	}

	outl(port+MBOXOUT0, KVTOPHYS(ecb));	/* don't know this will work */
	outb(port+ATTN, opcode|target);
	splx(s);
}

/*
 * Function to poll for command completion when in poll mode
 * wait is in msec
 */
int
ahb_poll(int unit, int wait)
{
	int port = ahb_data[unit].baseport;
	int spincount = FUDGE(delaycount) * wait; /* in msec */
	int stport = port + G2STAT;
	int start = spincount;

retry:
	while( spincount-- && (!(inb(stport) &  G2STAT_INT_PEND)))
		;
	if(spincount == -1) {
		printf("ahb%d: board not responding\n",unit);
		return(EIO);
	}
	if( (int)cheat != PHYSTOKV(inl(port+MBOXIN0)) ) {
		printf("discarding %x ", inl(port+MBOXIN0));
		outb(port + G2CNTRL, G2CNTRL_CLEAR_EISA_INT);
		spinwait(50);
		goto retry;
	}
	ahbintr(unit);
	return(0);
}
/*
 * Function to  send an immediate type command to the adapter
 */
void
ahb_send_immed(int unit, int target, u_long cmd)
{
	int port = ahb_data[unit].baseport;
	int spincount = FUDGE(delaycount) * 1; /* 1ms should be enough */
	int s = splbio();
	int stport = port + G2STAT;

	while( ((inb(stport) &
	    (G2STAT_BUSY | G2STAT_MBOX_EMPTY)) != (G2STAT_MBOX_EMPTY)) &&
	    spincount--)
		;
	if(spincount == -1) {
		printf("ahb%d: board not responding\n",unit);
		Debugger();
	}

	outl(port + MBOXOUT0, cmd);	/* don't know this will work */
	outb(port + G2CNTRL, G2CNTRL_SET_HOST_READY);
	outb(port + ATTN, OP_IMMED | target);
	splx(s);
}

/*
 * Check  the slots looking for a board we recognise
 * If we find one, note it's address (slot) and call
 * the actual probe routine to check it out.
 */
int
ahbprobe(struct isa_device *dev)
{
	int port;
	u_char	byte1,byte2,byte3;

	ahb_slot++;
	while (ahb_slot<8) {
		port = 0x1000 * ahb_slot;
		byte1 = inb(port + HID0);
		byte2 = inb(port + HID1);
		byte3 = inb(port + HID2);
		if(byte1 == 0xff) {
			ahb_slot++;
			continue;
		}
		if ((CHAR1(byte1,byte2) == 'A')
		    && (CHAR2(byte1,byte2) == 'D')
		    && (CHAR3(byte1,byte2) == 'P')
		    && ((byte3 == 0 ) || (byte3 == 1))) {
			dev->id_iobase = port;
			return ahbprobe1(dev);
		}
		ahb_slot++;
	}
	return 0;
}

/*
 * Check if the device can be found at the port given    *
 * and if so, set it up ready for further work           *
 * as an argument, takes the isa_device structure from      *
 * autoconf.c                                            *
 */
int
ahbprobe1(struct isa_device *dev)
{
	int unit = ahb_unit;

	dev->id_unit = unit;
	ahb_data[unit].baseport = dev->id_iobase;
	if(unit >= NAHB) {
		printf("ahb: unit number (%d) too high\n",unit);
		return 0;
	}

	/*
	 * Try initialise a unit at this location
	 * sets up dma and bus speed, loads ahb_data[unit].vect*
	 */
	if (ahb_init(unit) != 0)
		return 0;

	/* If it's there, put in it's interrupt vectors	*/
        dev->id_irq = (1 << ahb_data[unit].vect);
        dev->id_drq = -1;		/* using EISA dma */
	ahb_unit++;
	return 0x1000;
}

/*
 * Attach all the sub-devices we can find
 */
int
ahb_attach(struct isa_device *dev)
{
	static int firsttime;
	static int firstswitch[NAHB];
	int masunit = dev->id_masunit;
	int r;

	if (!firstswitch[masunit]) {
		firstswitch[masunit] = 1;
		ahb_switch[masunit].name = "ahb";
		ahb_switch[masunit].scsi_cmd = ahb_scsi_cmd;
		ahb_switch[masunit].scsi_minphys = ahbminphys;
		ahb_switch[masunit].open_target_lu = 0;
		ahb_switch[masunit].close_target_lu = 0;
		ahb_switch[masunit].adapter_info = ahb_adapter_info;
		for (r = 0; r < 8; r++) {
			ahb_switch[masunit].empty[r] = 0;
			ahb_switch[masunit].used[r] = 0;
			ahb_switch[masunit].printed[r] = 0;
		}
	}
	r = scsi_attach(masunit, ahb_data[masunit].our_id, &ahb_switch[masunit],
		&dev->id_physid, &dev->id_unit, dev->id_flags);

	/* only one for all boards */
	if(firsttime==0) {
		firsttime = 1;
		ahb_timeout(0);
	}
	return r;
}

/*
 * Return some information to the caller about   *
 * the adapter and it's capabilities             *
 * 2 outstanding requests at a time per device
 */
long int
ahb_adapter_info(int unit)
{
	return 2;
}

/*
 * Catch an interrupt from the adaptor
 */
int
ahbintr(int unit)
{
	struct ecb	 *ecb;
	unsigned char	stat;
	register	i;
	u_char		ahbstat;
	int		target;
	long int 	mboxval;

	int port = ahb_data[unit].baseport;

	if(scsi_debug & PRINTROUTINES)
		printf("ahbintr ");

	while(inb(port + G2STAT) & G2STAT_INT_PEND) {
		/*
		 * First get all the information and then 
		 * acknowlege the interrupt
		 */
		ahbstat = inb(port + G2INTST);
		target = ahbstat & G2INTST_TARGET;
		stat = ahbstat & G2INTST_INT_STAT;
		mboxval = inl(port + MBOXIN0);/* don't know this will work */
		outb(port + G2CNTRL, G2CNTRL_CLEAR_EISA_INT);
		if(scsi_debug & TRACEINTERRUPTS)
			printf("status = 0x%x ",stat);

		/*
		 * Process the completed operation
		 */
		if(stat == AHB_ECB_OK)
			ecb = (struct ecb *)PHYSTOKV(mboxval); 
		else {
			switch(stat) {
			case AHB_IMMED_OK:
				ecb = ahb_data[unit].immed_ecb;
				ahb_data[unit].immed_ecb = 0;
				break;
			case AHB_IMMED_ERR:
				ecb = ahb_data[unit].immed_ecb;
				ecb->flags |= ECB_IMMED_FAIL;
				ahb_data[unit].immed_ecb = 0;
				break;
			case AHB_ASN:	/* for target mode */
				ecb = 0;
				break;
			case AHB_HW_ERR:
				ecb = 0;
				break;
			case AHB_ECB_RECOVERED:
				ecb = (struct ecb *)PHYSTOKV(mboxval); 
				break;
			case AHB_ECB_ERR:
				ecb = (struct ecb *)PHYSTOKV(mboxval); 
				break;
			default:
				printf(" Unknown return from ahb%d(%x)\n",unit,ahbstat);
				ecb=0;
			}
		}
		if(ecb) {
			if(ahb_debug & AHB_SHOWCMDS )
				ahb_show_scsi_cmd(ecb->xs);
			if((ahb_debug & AHB_SHOWECBS) && ecb)
				printf("<int ecb(%x)>",ecb);
			ahb_remove_timeout(ecb);
			ahb_done(unit, ecb, (stat==AHB_ECB_OK)? SUCCESS: FAIL);
		}
	}
	return 1;
}

/*
 * We have a ecb which has been processed by the
 * adaptor, now we look to see how the operation
 * went.
 */
void
ahb_done(int unit, struct ecb *ecb, int state)
{
	struct ahb_ecb_status  *stat = &ecb->ecb_status;
	struct scsi_sense_data *s1,*s2;
	struct scsi_xfer *xs = ecb->xs;

	if(scsi_debug & (PRINTROUTINES | TRACEINTERRUPTS))
		printf("ahb_done ");

	/*
	 * Otherwise, put the results of the operation
	 * into the xfer and call whoever started it
	 */
	if(ecb->flags & ECB_IMMED) {
		if(ecb->flags & ECB_IMMED_FAIL)
			xs->error = XS_DRIVER_STUFFUP;
		goto done;
	}
	if ( (state == SUCCESS) || (xs->flags & SCSI_ERR_OK)) {
		/* All went correctly  OR errors expected */
		xs->resid = 0;
		xs->error = 0;
	} else {
		s1 = &(ecb->ecb_sense);
		s2 = &(xs->sense);

		if(stat->ha_status) {
			switch(stat->ha_status) {
			case HS_SCSI_RESET_ADAPTER:
				break;
			case HS_SCSI_RESET_INCOMING:
				break;
			case HS_CMD_ABORTED_HOST:	/* No response */
			case HS_CMD_ABORTED_ADAPTER:	/* No response */
				break;
			case HS_TIMED_OUT:		/* No response */
				if (ahb_debug & AHB_SHOWMISC)
					printf("timeout reported back\n");
				xs->error = XS_TIMEOUT;
				break;
			default:
				/* Other scsi protocol messes */
				xs->error = XS_DRIVER_STUFFUP;
				if (ahb_debug & AHB_SHOWMISC)
					printf("unexpected ha_status: %x\n",
						stat->ha_status);
			}

		} else {
			switch(stat->targ_status) {
			case TS_CHECK_CONDITION:
				*s2 = *s1;
				xs->error = XS_SENSE;
				break;
			case TS_BUSY:
				xs->error = XS_BUSY;
				break;
			default:
				if (ahb_debug & AHB_SHOWMISC)
					printf("unexpected targ_status: %x\n",
						stat->targ_status);
				xs->error = XS_DRIVER_STUFFUP;
			}
		}
	}

done:
	xs->flags |= ITSDONE;
	ahb_free_ecb(unit, ecb, xs->flags);
	if(xs->when_done)
		(*(xs->when_done))(xs->done_arg,xs->done_arg2);
}

/*
 * A ecb (and hence a mbx-out is put onto the 
 * free list.
 */
void
ahb_free_ecb(int unit, struct ecb *ecb, int flags)
{
	unsigned int opri;

	if(scsi_debug & PRINTROUTINES)
		printf("ecb%d(0x%x)> ",unit,flags);
	if (!(flags & SCSI_NOMASK)) 
	  	opri = splbio();

	ecb->next = ahb_data[unit].free_ecb;
	ahb_data[unit].free_ecb = ecb;
	ecb->flags = ECB_FREE;

	/*
	 * If there were none, wake abybody waiting for
	 * one to come free, starting with queued entries*
	 */
	if (!ecb->next)
		wakeup((caddr_t)&ahb_data[unit].free_ecb);

	if (!(flags & SCSI_NOMASK)) 
		splx(opri);
}

/*
 * Get a free ecb (and hence mbox-out entry)
 */
struct ecb *
ahb_get_ecb(int unit, int flags)
{
	unsigned opri;
	struct ecb *rc;

	if(scsi_debug & PRINTROUTINES)
		printf("<ecb%d(0x%x) ", unit, flags);
	if (!(flags & SCSI_NOMASK)) 
	  	opri = splbio();

	/*
	 * If we can and have to, sleep waiting for one
	 * to come free
	 */
	while ((!(rc = ahb_data[unit].free_ecb)) && (!(flags & SCSI_NOSLEEP)))
		sleep((caddr_t)&ahb_data[unit].free_ecb, PRIBIO);

	if (rc) {
		ahb_data[unit].free_ecb = rc->next;
		rc->flags = ECB_ACTIVE;
	}

	if (!(flags & SCSI_NOMASK)) 
		splx(opri);
	return rc;
}

/*
 * Start the board, ready for normal operation
 */
int
ahb_init(int unit)
{
	int port = ahb_data[unit].baseport;
	int intdef;
	int spincount = FUDGE(delaycount) * 1000; /* 1 sec enough? */
	int i;
	int stport = port + G2STAT;

#define	NO_NO 1
#ifdef NO_NO
	/*
	 * reset board, If it doesn't respond, assume 
	 * that it's not there.. good for the probe
	 */
	outb(port + EBCTRL,CDEN);	/* enable full card */
	outb(port + PORTADDR,PORTADDR_ENHANCED);

	outb(port + G2CNTRL,G2CNTRL_HARD_RESET);
	spinwait(1);
	outb(port + G2CNTRL,0);
	spinwait(10);
	while( (inb(stport) & G2STAT_BUSY ) && spincount--)
		;
	if(spincount == -1) {
		if (ahb_debug & AHB_SHOWMISC)
			printf("ahb_init: No answer from bt742a board\n");
		return(ENXIO);
	}

	i = inb(port + MBOXIN0) & 0xff;
	if(i) {
		printf("self test failed, val = 0x%x\n",i);
		return(EIO);
	}
#endif

	while( inb(stport) &  G2STAT_INT_PEND) {
		printf(".");
		outb(port + G2CNTRL, G2CNTRL_CLEAR_EISA_INT);
		spinwait(10);
	}
	outb(port + EBCTRL,CDEN);	/* enable full card */
	outb(port + PORTADDR,PORTADDR_ENHANCED);

	/*
	 * Assume we have a board at this stage
	 * setup dma channel from jumpers and save int
	 * level
	 */

	intdef = inb(port + INTDEF);
	switch(intdef & 0x07) {
	case INT9:
		ahb_data[unit].vect = 9;
		break;
	case INT10:
		ahb_data[unit].vect = 10;
		break;
	case INT11:
		ahb_data[unit].vect = 11;
		break;
	case INT12:
		ahb_data[unit].vect = 12;
		break;
	case INT14:
		ahb_data[unit].vect = 14;
		break;
	case INT15:
		ahb_data[unit].vect = 15;
		break;
	default:
		ahb_data[unit].vect = -1;
		printf("ahb%d: illegal irq setting\n", unit);
		return(EIO);
	}

	outb(port + INTDEF, intdef|INTEN); /* make sure we can interrupt */
	ahb_data[unit].our_id = (inb(port + SCSIDEF) & HSCSIID);

	/*
	 * link up all our ECBs into a free list
	 */
	for (i=0; i < NUM_CONCURRENT; i++) {
		ahb_data[unit].ecbs[i].next = ahb_data[unit].free_ecb;
		ahb_data[unit].free_ecb = &ahb_data[unit].ecbs[i];
		ahb_data[unit].free_ecb->flags = ECB_FREE;
	}

	/*
	 * Note that we are going and return (to probe)
	 */
	ahb_data[unit].flags |= AHB_INIT;
	return 0;
}

void
ahbminphys(struct buf *bp)
{
	if(bp->b_bcount > ((AHB_NSEG-1) * PAGESIZ))
		bp->b_bcount = ((AHB_NSEG-1) * PAGESIZ);
}

/*
 * start a scsi operation given the command and
 * the data address. Also needs the unit, target
 * and lu
 */
int
ahb_scsi_cmd(struct scsi_xfer *xs)
{
	struct scsi_sense_data *s1,*s2;
	struct ecb *ecb;
	struct ahb_dma_seg *sg;
	int	seg;	/* scatter gather seg being worked on */
	int i	= 0;
	int rc	=  0;
	int	thiskv;
	physaddr	thisphys,nextphys;
	int	unit =xs->adapter;
	int	bytes_this_seg,bytes_this_page,datalen,flags;
	struct iovec	 *iovp;
	int	s;
	if(scsi_debug & PRINTROUTINES)
		printf("ahb_scsi_cmd ");
	/*
	 * get a ecb (mbox-out) to use. If the transfer
	 * is from a buf (possibly from interrupt time)
	 * then we can't allow it to sleep
	 */
	flags = xs->flags;
	if(xs->bp) flags |= (SCSI_NOSLEEP); /* just to be sure */
	if(flags & ITSDONE) {
		printf("Already done?");
		xs->flags &= ~ITSDONE;
	}
	if( !(flags & INUSE) ) {
		printf("Not in use?");
		xs->flags |= INUSE;
	}
	if (!(ecb = ahb_get_ecb(unit,flags))) {
		xs->error = XS_DRIVER_STUFFUP;
		return(TRY_AGAIN_LATER);
	}

cheat = ecb;

	if(ahb_debug & AHB_SHOWECBS)
		printf("<start ecb(%x)>",ecb);
	if(scsi_debug & SHOWCOMMANDS)
		ahb_show_scsi_cmd(xs);

	ecb->xs = xs;
	/*
	 * If it's a reset, we need to do an 'immediate'
	 * command, and store it's ccb for later
	 * if there is already an immediate waiting, 
	 * then WE must wait
	 */
	if(flags & SCSI_RESET) {
		ecb->flags |= ECB_IMMED;
		if(ahb_data[unit].immed_ecb)
			return(TRY_AGAIN_LATER);

		ahb_data[unit].immed_ecb = ecb;
		if (!(flags & SCSI_NOMASK)) {
			s = splbio();
			ahb_send_immed(unit,xs->targ,AHB_TARG_RESET);
			ahb_add_timeout(ecb,xs->timeout);
			splx(s);
			return(SUCCESSFULLY_QUEUED);
		} else {
			ahb_send_immed(unit,xs->targ,AHB_TARG_RESET);
			/*
			 * If we can't use interrupts, poll on completion*
			 */
			if(scsi_debug & TRACEINTERRUPTS)
				printf("wait ");
			if( ahb_poll(unit,xs->timeout)) {
				ahb_free_ecb(unit,ecb,flags);
				xs->error = XS_TIMEOUT;
				return(HAD_ERROR);
			}
			return(COMPLETE);
		}
	}
	/*
	 * Put all the arguments for the xfer in the ecb
	 */
	ecb->opcode = ECB_SCSI_OP;
	ecb->opt1 = ECB_SES|ECB_DSB|ECB_ARS;
	if(xs->datalen)
		ecb->opt1 |= ECB_S_G;
	ecb->opt2 		=	xs->lu | ECB_NRB;
	ecb->cdblen		=	xs->cmdlen;
	ecb->sense		=	KVTOPHYS(&(ecb->ecb_sense));
	ecb->senselen		=	sizeof(ecb->ecb_sense);
	ecb->status		=	KVTOPHYS(&(ecb->ecb_status));

	if(xs->datalen) {
		/* should use S/G only if not zero length */
		ecb->data 	=	KVTOPHYS(ecb->ahb_dma);
		sg		=	ecb->ahb_dma ;
		seg 		=	0;
		if(flags & SCSI_DATA_UIO) {
			iovp = ((struct uio *)xs->data)->uio_iov;
			datalen = ((struct uio *)xs->data)->uio_iovcnt;
			xs->datalen = 0;
			while ((datalen) && (seg < AHB_NSEG)) {
				sg->addr = (physaddr)iovp->iov_base;
				xs->datalen += sg->len = iovp->iov_len;
				if(scsi_debug & SHOWSCATGATH)
					printf("(0x%x@0x%x)", iovp->iov_len,
						iovp->iov_base);
				sg++;
				iovp++;
				seg++;
				datalen--;
			}
		} else {
			/* Set up the scatter gather block */

			if(scsi_debug & SHOWSCATGATH)
				printf("%d @0x%x:- ", xs->datalen, xs->data);
			datalen		=	xs->datalen;
			thiskv		=	(int)xs->data;
			thisphys	=	KVTOPHYS(thiskv);

			while ((datalen) && (seg < AHB_NSEG)) {
				bytes_this_seg	= 0;

				/* put in the base address */
				sg->addr = thisphys;

				if(scsi_debug & SHOWSCATGATH)
					printf("0x%x",thisphys);

				/* do it at least once */
				nextphys = thisphys;
				while ((datalen) && (thisphys == nextphys)) {
				/*
				 * This page is contiguous (physically) with   *
				 * the the last, just extend the length	      *
				 */
					nextphys= (thisphys & (~(PAGESIZ - 1)))
								+ PAGESIZ;
					bytes_this_page	= min(nextphys - thisphys,
						datalen);
					bytes_this_seg	+= bytes_this_page;
					datalen		-= bytes_this_page;

					/* get more ready for the next page */
					thiskv	= (thiskv & (~(PAGESIZ - 1)))
								+ PAGESIZ;
					if(datalen)
						thisphys = KVTOPHYS(thiskv);
				}
				/*
				 * next page isn't contiguous, finish the seg *
				 */
				if(scsi_debug & SHOWSCATGATH)
					printf("(0x%x)",bytes_this_seg);
				sg->len = bytes_this_seg;
				sg++;
				seg++;
			}
		}
		ecb->datalen = seg * sizeof(struct ahb_dma_seg);
		if(scsi_debug & SHOWSCATGATH)
			printf("\n");
		if (datalen) {
			/* there's still data, must have run out of segs! */
			printf("ahb_scsi_cmd%d: more than %d DMA segs\n",
				unit, AHB_NSEG);
			xs->error = XS_DRIVER_STUFFUP;
			ahb_free_ecb(unit,ecb,flags);
			return(HAD_ERROR);
		}

	} else {
		/* No data xfer, use non S/G values */
		ecb->data = (physaddr)0;
		ecb->datalen = 0;
	}

	ecb->chain = (physaddr)0;
	/*
	 * Put the scsi command in the ecb and start it
	 */
	bcopy(xs->cmd, ecb->cdb, xs->cmdlen);

	/* Usually return SUCCESSFULLY QUEUED */
	if( !(flags & SCSI_NOMASK) ) {
		s = splbio();
		ahb_send_mbox(unit,OP_START_ECB,xs->targ,ecb);
		ahb_add_timeout(ecb,xs->timeout);
		splx(s);
		if(scsi_debug & TRACEINTERRUPTS)
			printf("cmd_sent ");
		return(SUCCESSFULLY_QUEUED);
	}

	/* If we can't use interrupts, poll on completion */
	ahb_send_mbox(unit,OP_START_ECB,xs->targ,ecb);
	if(scsi_debug & TRACEINTERRUPTS)
		printf("cmd_wait ");

	do {
		if(ahb_poll(unit,xs->timeout)) {
			if (!(xs->flags & SCSI_SILENT)) printf("cmd fail\n");
			ahb_send_mbox(unit,OP_ABORT_ECB,xs->targ,ecb);
			if(ahb_poll(unit, 2000)) {
				printf("abort failed in wait\n");
				ahb_free_ecb(unit,ecb,flags);
			}
			xs->error = XS_DRIVER_STUFFUP;
			return(HAD_ERROR);
		}
	} while (!(xs->flags & ITSDONE));

	scsi_debug = 0;
	ahb_debug = 0;
	if(xs->error)
		return HAD_ERROR;
	return COMPLETE;
}

/*
 *                +----------+    +----------+    +----------+
 * ahb_soonest--->|    later |--->|     later|--->|     later|--->0
 *                | [Delta]  |    | [Delta]  |    | [Delta]  |
 *           0<---|sooner    |<---|sooner    |<---|sooner    |<---ahb_latest
 *                +----------+    +----------+    +----------+
 *
 *     ahb_furtherest = sum(Delta[1..n])
 */
void
ahb_add_timeout(struct ecb *ecb, int time)
{
	int	timeprev;
	struct ecb *prev;
	int	s = splbio();

	prev = ahb_latest;
	if(prev)
		timeprev = ahb_furtherest;
	else
		timeprev = 0;

	while(prev && (timeprev > time)) {
		timeprev -= prev->delta;
		prev = prev->sooner;
	}
	if(prev) {
		ecb->delta = time - timeprev;
		ecb->later = prev->later;
		if(ecb->later) {
			ecb->later->sooner = ecb;
			ecb->later->delta -= ecb->delta;
		} else {
			ahb_furtherest = time;
			ahb_latest = ecb;
		}
		ecb->sooner = prev;
		prev->later = ecb;
	}
	else
	{
		ecb->later = ahb_soonest;
		if(ahb_soonest) {
			ecb->later->sooner = ecb;
			ecb->later->delta -= time;
		} else {
			ahb_furtherest = time;
			ahb_latest = ecb;
		}
		ecb->delta = time;
		ecb->sooner = (struct ecb *)0;
		ahb_soonest = ecb;
	}
	splx(s);
}

void
ahb_remove_timeout(struct ecb *ecb)
{
	int	s = splbio();

	if(ecb->sooner)
		ecb->sooner->later = ecb->later;
	else
		ahb_soonest = ecb->later;

	if(ecb->later) {
		ecb->later->sooner = ecb->sooner;
		ecb->later->delta += ecb->delta;
	} else {
		ahb_latest = ecb->sooner;
		ahb_furtherest -= ecb->delta;
	}
	ecb->sooner = ecb->later = (struct ecb *)0;
	splx(s);
}


extern int 	hz;
#define ONETICK 500 /* milliseconds */
#define SLEEPTIME ((hz * 1000) / ONETICK)

void
ahb_timeout(int arg)
{
	struct ecb *ecb;
	int unit;
	int s = splbio();

	while( ecb = ahb_soonest ) {
		if(ecb->delta <= ONETICK) {
			/* It has timed out, we need to do some work */
			unit = ecb->xs->adapter;
			printf("ahb%d targ %d: device timed out\n", unit,
				ecb->xs->targ);
			if(ahb_debug & AHB_SHOWECBS)
				ahb_print_active_ecb();

			/* Unlink it from the queue */
			ahb_remove_timeout(ecb);

			/*
			 * If it's immediate, don't try abort it *
			 */
			if(ecb->flags & ECB_IMMED) {
				ecb->xs->retries = 0; /* I MEAN IT ! */
				ecb->flags |= ECB_IMMED_FAIL;
                                ahb_done(unit,ecb,FAIL);
				continue;
			}

			/*
			 * If it has been through before, then
			 * a previous abort has failed, don't
			 * try abort again
			 */
			if(ecb->flags == ECB_ABORTED) {
				printf("AGAIN");
				ecb->xs->retries = 0; /* I MEAN IT ! */
				ecb->ecb_status.ha_status = HS_CMD_ABORTED_HOST;
				ahb_done(unit,ecb,FAIL);
			} else {
				printf("\n");
				ahb_send_mbox(unit,OP_ABORT_ECB,ecb->xs->targ,ecb);
					/* 2 secs for the abort */
				ahb_add_timeout(ecb,2000 + ONETICK);
				ecb->flags = ECB_ABORTED;
			}
		} else {
			ecb->delta -= ONETICK;
			ahb_furtherest -= ONETICK;
			break;
		}
	}
	splx(s);
	timeout((timeout_t)ahb_timeout,(caddr_t)arg,SLEEPTIME);
}

void
ahb_show_scsi_cmd(struct scsi_xfer *xs)
{
	u_char  *b = (u_char *)xs->cmd;
	int i = 0;

	if( !(xs->flags & SCSI_RESET) ) {
		printf("ahb%d targ %d lun %d:", xs->adapter,
			xs->targ, xs->lu);
		while(i < xs->cmdlen ) {
			if(i)
				printf(",");
			printf("%x", b[i++]);
		}
		printf("\n");
	} else {
		printf("ahb%d targ %d lun%d: RESET\n", xs->adapter,
			xs->targ, xs->lu);
	}
}

void
ahb_print_ecb(struct ecb *ecb)
{
	printf("ecb:%x op:%x cmdlen:%d senlen:%d\n", ecb, ecb->opcode,
		ecb->cdblen, ecb->senselen);
	printf("    datlen:%d hstat:%x tstat:%x delta:%d flags:%x\n",
		ecb->datalen, ecb->ecb_status.ha_status,
		ecb->ecb_status.targ_status, ecb->delta, ecb->flags);
	ahb_show_scsi_cmd(ecb->xs);
}

void
ahb_print_active_ecb(void)
{
	struct ecb *ecb;
	ecb = ahb_soonest;

	while(ecb) {
		ahb_print_ecb(ecb);
		ecb = ecb->later;
	}
	printf("Furtherest = %d\n", ahb_furtherest);
}
