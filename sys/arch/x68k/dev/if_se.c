/*	$NetBSD: if_se.c,v 1.8 1999/05/18 23:52:55 thorpej Exp $	*/

/*
 * Device driver for National Semiconductor DS8390 based ethernet adapters.
 *
 * Based on original ISA bus driver by David Greenman, 29-April-1993
 *
 * Copyright (C) 1993, David Greenman. This software may be used, modified,
 *   copied, distributed, and sold, in both source and binary form provided
 *   that the above copyright and these terms are retained. Under no
 *   circumstances is the author responsible for the proper functioning
 *   of this software, nor does the author assume any responsibility
 *   for damages incurred with its use.
 *
 * Adapted for MacBSD by Brad Parker <brad@fcr.com>
 *
 * Currently supports:
 *	Cayman Systems GatorCard
 */

#include "opt_inet.h"
#include "opt_ns.h"

#include "se.h"
#if NSE > 0
/* bpfilter included here in case it is needed in future net includes */
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/buf.h>	/* XXX */
#include <sys/dkstat.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/netisr.h>
#include <sys/malloc.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <x68k/dev/if_sereg.h>
#include <x68k/dev/device.h>
#include <x68k/dev/scsireg.h>
#include <x68k/dev/scsivar.h>

/*
 * se_softc: per line info and status
 */
struct	se_softc {
	struct	x68k_device *sc_xd;
	struct	scsi_queue sc_dq;
	int	sc_flags;
	short	sc_type;	/* ansi scsi type */
	short	sc_punit;	/* physical unit (scsi lun) */
	u_char	sc_cmd;

	char	sc_datalen[32];	/* additional data length on some commands */

	struct	arpcom sc_arpcom;	/* ethernet common */

	char	*type_str;	/* pointer to type string */
	u_char	vendor;		/* interface vendor */
	u_char	type;		/* interface type code */
	caddr_t	smem_start;	/* shared memory start address */
	caddr_t	smem_end;	/* shared memory end address */
	u_long	smem_size;	/* total shared memory size */
	caddr_t	smem_ring;	/* start of RX ring-buffer (in smem) */

	caddr_t	bpf;		/* BPF "magic cookie" */

	u_char	xmit_busy;	/* transmitter is busy */
	u_char	txb_cnt;	/* Number of transmit buffers */
	u_char	txb_next;	/* Pointer to next buffer ready to xmit */
	u_short	txb_next_len;	/* next xmit buffer length */
	u_char	txb_inuse;	/* data has been buffered in interface memory */
	u_char	tx_page_start;	/* first page of TX buffer area */

	u_char	rec_page_start;	/* first page of RX ring-buffer */
	u_char	rec_page_stop;	/* last page of RX ring-buffer */
	u_char txbuf[1550];
	u_char rxbuf[1550];
} se_softc[NSE];

/* softc flags */
#define SEF_ALIVE	0x0001
#define SEF_OPEN	0x0002
#define SEF_WMODE	0x0004
#define SEF_WRTTN	0x0008
#define SEF_CMD		0x0010
#define SEF_LEOT	0x0020
#define SEF_MOVED	0x0040
#define SEF_RESET	0x0080

/* xsense sense key */
#define XSK_NOSENCE	0x0
#define XSK_NOTUSED1	0x1
#define XSK_NOTRDY	0x2
#define XSK_MEDERR	0x3
#define XSK_HRDWERR	0x4
#define XSK_ILLREQ	0x5
#define XSK_UNTATTEN	0x6
#define XSK_DATAPROT	0x7
#define XSK_BLNKCHK	0x8
#define XSK_VENDOR	0x9
#define XSK_CPYABORT	0xa
#define XSK_ABORTCMD	0xb
#define XSK_NOTUSEDC	0xc
#define XSK_VOLOVER	0xd
#define XSK_NOTUSEDE	0xe
#define XSK_REVERVED	0xf

#define	EPCLEARTALLIES		0
#define	EPSENDETHERPACKET	1
#define	EPRECIEVEETHERPACKET	2
#define	EPSETETHERADDR		3
#define	EPSETMULTICASTADDR	4
#define	EPSETPACKETRECEPTMODE	5
#define	EPETHERNETRECIEVE	6

int seident __P((register struct se_softc *, register struct x68k_device *));
int sefind __P((struct x68k_device *xd));
void seintr __P((register int unit, int stat));
void	se_init __P((struct se_softc *));
int	se_reset __P((struct se_softc *));
void	se_stop __P((struct se_softc *));
int	se_start __P((struct ifnet *));
int	se_ioctl __P((struct ifnet *, int, caddr_t));
int	se_probe();
void sestart __P((register int unit));
int	se_watchdog __P((struct ifnet *));
void sestrategy __P((struct buf *bp));
static int seerror __P((int unit, register struct se_softc *sc, register struct\
 x68k_device *am, int stat));
static void sefinish __P((int unit, register struct se_softc *sc, register stru\
ct buf *bp));
void seustart __P((register int unit));

extern int scsireq();
extern void	scsistart();
extern void disksort();

static struct scsi_fmt_cdb secmd[NSE];
struct  scsi_fmt_sense sesense[NSE];

#if 0
static struct scsi_fmt_cdb se_read_cmd = { 6, CMD_READ };
static struct scsi_fmt_cdb se_write_cmd = { 6, CMD_WRITE };
#endif

static struct scsi_fmt_cdb se_setmcast = {
	6,
	CMD_SET_MULTICASTADDR, 0, 0, 0, 8, 0
};
static struct scsi_fmt_cdb se_setprm = {
	6,
	CMD_SET_PACKETRECEPTMODE, 0, 0, 0, 0, 0
};
static struct scsi_fmt_cdb se_sendpkt = {
	6,
	CMD_SEND_ETHERPACKET, 0, 0, 0, 0, 0
};
static struct scsi_fmt_cdb se_recpkt = {
	6,
	CMD_RECIEVE_ETHERPACKET, 0x1f, 0xff, 0xff, 0xff, 0
};
u_char mcastaddr[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

struct buf setab[NSE];

#define seunit(x)	(minor(x) & 3)
#define sepunit(x)	((x) & 7)


#ifdef DEBUG
#define SE_OPEN		0x0001
#define SE_GO		0x0002
#define SE_FMKS		0x0004
#define SE_OPENSTAT	0x0008
#define SE_BRESID	0x0010
#define SE_ODDIO	0x0020
#define SEB_ERROR	0x0040
int sedebug = SE_ODDIO|SE_BRESID|SE_GO|SEB_ERROR;
#endif

struct	driver sedriver = {
	sefind, "se", (int (*)())0, (int (*)())0,
	(int (*)())seintr,
	(int (*)())0
};

static inline void se_rint();
static inline void se_xmit();
static inline char *se_ring_copy();

char se_name[] = "DP8390 SCSI Ethernet Adapter (Ether+)";
static char zero = 0;
static u_char ones = 0xff;

/*
 * XXX These two should be moved to locore, and maybe changed to use shorts
 * instead of bytes.  The reason for these is that bcopy and bzero use longs,
 * which the ethernet cards can't handle.
 */

void
bbcopy (char *src, char *dest, int len)
{
	while (len--) {
		*dest++ = *src++;
	}
}

int
sefind(xd)
	register struct x68k_device *xd;
{
	register struct se_softc *sc = &se_softc[xd->x68k_unit];
	struct buf *bp;
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;
	struct ifaddr *ifa;
	struct sockaddr_dl *sdl;
	int stat;
	int s, i;

	for (bp = setab; bp < &setab[NSE]; bp++)
		bp->b_actb = &bp->b_actf;

	/*
	 * I seem to be interrupted every leap year while in here..
	 *
	 * Note: do this *only* if you don't need interrupt driven
	 *       SCSI in here. We currently don't, as all I/O in here
	 *	 is done with PIO, but beware..
	 */
	sc->sc_xd = xd;
	sc->sc_punit = sepunit(xd->x68k_flags);
	sc->sc_type = seident(sc, xd);
	if (sc->sc_type < 0)
		return(0);
	sc->sc_dq.dq_ctlr = xd->x68k_ctlr;
	sc->sc_dq.dq_unit = xd->x68k_unit;
	sc->sc_dq.dq_slave = xd->x68k_slave;
	sc->sc_dq.dq_driver = &sedriver;

	sc->sc_flags = SEF_ALIVE;

	/*
	 * Initialize ifnet structure
	 */
	/* XXX: se->se_dev.dv_unit, se_cd.cd_name */
	sprintf(ifp->if_xname, "%s%d", sedriver.d_name, xd->x68k_unit);
	ifp->if_softc = sc;
	ifp->if_start = se_start;
	ifp->if_ioctl = se_ioctl;
	ifp->if_watchdog = se_watchdog;
	ifp->if_flags = (IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS | IFF_MULTICAST);

	/*
	 * Attach the interface
	 */
	if_attach(ifp);
	ether_ifattach(ifp);

#if NBPFILTER > 0
	bpfattach(&ifp->if_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif
	/*
	 * Print additional info when attached
	 */
	printf(" address %s\n", ether_sprintf(sc->sc_arpcom.ac_enaddr));

	stat = scsi_scsi_cmd(xd->x68k_ctlr, xd->x68k_slave, xd->x68k_unit, &se_setmcast, 
			     mcastaddr, 8);
	if (stat != 0)
	  printf("sefind:WARNING!! SET_MULTICAST_ADDR FAILED(stat == %d)\n", stat);

	stat = scsi_scsi_cmd(xd->x68k_ctlr, xd->x68k_slave, xd->x68k_unit,
			     &se_setprm, NULL, 0);
	if (stat != 0)
	  printf("sefind:WARNING!! SET_PACKET_RECEPT_MODE FAILED(stat == %d)\n", stat);

	se_setprm.cdb[2] = 0x04;
	stat = scsi_scsi_cmd(xd->x68k_ctlr, xd->x68k_slave, xd->x68k_unit,
			     &se_setprm, NULL, 0);
	if (stat != 0)
	  printf("sefind:WARNING!! SET_PACKET_RECEPT_MODE FAILED(stat == %d)\n", stat);
	return(1);
}

void
se_ethernet_read_setup(sc)
	register struct se_softc *sc;
{
	int unit;
	int ctlr, slave;

	ctlr = sc->sc_xd->x68k_ctlr;
	slave = sc->sc_xd->x68k_slave;
	unit = sc->sc_punit;
	scsi_scsi_cmd(ctlr, slave, unit, &se_recpkt, sc->rxbuf, 0);
}

int
seident(sc, xd)
	register struct se_softc *sc;
	register struct x68k_device *xd;
{
	int unit;
	int ctlr, slave;
	int i, stat, inqlen;
	char idstr[32];
	u_int memsize;
	int flags = 0;
	char    manu[32];
	char    model[32];
	char    version[32];
	register int tries = 10;

	struct  etherplus_inquiry { 
		u_char	eaddr[6];
		u_char	filler0[14];
		u_char	serialno[12];
		u_char	filler1[28];
	};
	struct se_inquiry {
		struct	scsi_inquiry inqbuf;
		struct  etherplus_inquiry ep_inquiry;
	} se_inqbuf;
	static struct scsi_fmt_cdb se_inq = {
		6,
		CMD_INQUIRY, 0, 0, 0, sizeof(se_inqbuf), 0
	};

	ctlr = xd->x68k_ctlr;
	slave = xd->x68k_slave;
	unit = sc->sc_punit;

	inqlen = 0x05; /* min */
	se_inq.cdb[4] = 0x05;
	if (scsi_inquire(ctlr, slave, unit, (struct scsi_inquiry *)&se_inqbuf))
		goto failed;

	if (se_inqbuf.inqbuf.type != 0x09)  /* not communication device */
		goto failed;

	/* now get additonal info */
	inqlen = 0x60;		/* Inquiry returns inqbuf.len == 0. (T_T) */
	se_inq.cdb[4] = inqlen;
	bzero(&se_inqbuf, sizeof(se_inqbuf));
	while ((stat = scsi_scsi_cmd(ctlr, slave, unit, &se_inq,
				     (u_char *)&se_inqbuf, inqlen)) == SCSI_BUSY) {
		DELAY(10000);
        }
	if (stat == 0xff) { 
		printf("se%d: Cant handle this Ethernet device\n", xd->x68k_unit);
		goto failed;
	}
	if (stat != 0)
		goto failed;

	if (inqlen >= 32) {
		strncpy(manu, se_inqbuf.inqbuf.vendor_id, 8);
		for (i = 7; i >= 0; --i)
			if (manu[i] != ' ')
				break;
		manu[i+1] = 0;
		strncpy(model, se_inqbuf.inqbuf.product_id, 16);
		for (i = 7; i > 0; --i)
			if (model[i] != ' ')
				break;
		model[i+1] = 0;
		strncpy(version, se_inqbuf.inqbuf.rev, 4);
		for (i = 3; i > 0; --i)
			if (version[i] != ' ')
				break;
		version[i+1] = 0;
		printf("se%d: %s %s rev %s,", xd->x68k_unit, manu, model, version);
	} else if (inqlen == 5)
		/* great it's a stupid device, doesn't know it's know name */
		idstr[0] = idstr[8] = '\0';
	else
		idstr[8] = '\0';

	if (bcmp("Ether+", model, 6) == 0) {
		memsize = 8192;		/* XXX */
		sc->type_str = "Ether+";
		sc->sc_datalen[CMD_REQUEST_SENSE] = 26;
		sc->sc_datalen[CMD_INQUIRY] = 96;
		sc->sc_datalen[CMD_MODE_SELECT] = 17;
		sc->sc_datalen[CMD_MODE_SENSE] = 17;
		sc->txb_cnt = 1;	/* XXX */

		sc->tx_page_start = 0;
		for (i = 0; i < ETHER_ADDR_LEN; ++i)
			sc->sc_arpcom.ac_enaddr[i] = se_inqbuf.ep_inquiry.eaddr[i];
	} else {
		if (idstr[8] == '\0')
			printf("se%d: No ID, assuming Archive\n", xd->x68k_unit);
		else
			printf("se%d: Unsupported Ethernet device\n", xd->x68k_unit);
		sc->sc_datalen[CMD_REQUEST_SENSE] = 8;
		sc->sc_datalen[CMD_INQUIRY] = 5;
		sc->sc_datalen[CMD_MODE_SELECT] = 12;
		sc->sc_datalen[CMD_MODE_SENSE] = 12;
	}

	return(se_inqbuf.inqbuf.type);
failed:
	return(-1);
}

void
sestrategy(bp)
	register struct buf *bp;
{
	register int unit = seunit(bp->b_dev);
	register struct buf *dp = &setab[unit];
	int s;

	printf("sestrategy()\n");

#if 0
	bp->b_actf = NULL;
	s = splbio();
	bp->b_actb = dp->b_actb;
	*dp->b_actb = bp;
	dp->b_actb = &bp->b_actf;
#else
	s = splbio();
	disksort(dp, bp);
#endif
	if (dp->b_active == 0) {
		dp->b_active = 1;
		seustart(unit);
	}
	splx(s);
}

void
seustart(unit)
	register int unit;
{
   	register struct se_softc *sc = &se_softc[unit];
	register struct scsi_queue *dq = &sc->sc_dq;
	register struct buf *bp = setab[unit].b_actf;
	register struct scsi_fmt_cdb *cmd = &secmd[sc->sc_punit];
	long nblks;

	printf("seustart(%d)\n", unit);

	if (bp->b_flags & B_READ)
		sc->sc_flags &= ~SEF_WRTTN;
	else
		sc->sc_flags |= SEF_WRTTN;

	nblks = bp->b_bcount >> DEV_BSHIFT;

	dq->dq_cdb = cmd;
	dq->dq_bp  = bp;
        dq->dq_flags = DQ_DISCONNECT;   /* SCSI Disconnect */

	if (scsireq(dq)) {
		sestart(unit);
	}
	else {
		printf("seustart: scsireq() returns 0.\n");
	}
}

/*
 * Return:
 *	0	if not really an error
 *	<0	if we should do a retry
 *	>0	if a fatal error
 */
seerror(unit, sc, am, stat)
	int unit, stat;
	struct se_softc *sc;
        register struct x68k_device *am;
{
        int cond = 1;

        sesense[unit].status = stat;
	/* sexsense must have been called before seerror() */
	if (stat & STS_CHECKCOND) {
                struct scsi_xsense *sp;

		scsi_request_sense(am->x68k_ctlr, am->x68k_slave,
				   sc->sc_punit, sesense[unit].sense,
				   8/*sizeof(sesense[unit].sense)*/);
		sp = (struct scsi_xsense *)sesense[unit].sense;
		/* prtkey(unit, sc); */
                printf("se%d: scsi sense class %d, code %d", unit,
                        sp->class, sp->code);
		if (sp->class == 7) {
			printf(", key %d", sp->key);
			if (sp->valid)
				printf(", blk %d", *(int *)&sp->info1);
			switch (sp->key) {
			/* no sense, try again */
			case 0:
				cond = -1;
				break;
			/* recovered error, not a problem */
			case 1:
				cond = 0;
				break;
			/* else, unrecovered error */
			}
		}
		printf("\n");
	}
	return(cond);
}

void
sestart(unit)
	register int unit;
{
	struct x68k_device *am = se_softc[unit].sc_xd;

	printf("sestart(%d)\n", unit);

	if (am->x68k_dk >= 0) {
		dk_busy |= 1 << am->x68k_dk;
	}
	scsistart(am->x68k_ctlr);
}

sexsense(ctlr, slave, unit, sc)
	int ctlr, slave, unit;
	struct se_softc *sc;
{
	struct scsi_xsense *sensebuf;
	unsigned len;
	int s;

	len = sc->sc_datalen[CMD_REQUEST_SENSE];
	scsi_request_sense(ctlr, slave, unit, sesense[unit].sense, len);
}

void
sefinish(unit, sc, bp)
	int unit;
	register struct se_softc *sc;
	register struct buf *bp;
{
	struct buf *dp;

	printf("sefinish(%d)\n", unit);
#if 0
	setab[unit].b_errcnt = 0;
	if (dp = bp->b_actf)
		dp->b_actb = bp->b_actb;
	else
		setab[unit].b_actb = bp->b_actb;
	*bp->b_actb = dp;
#else
	setab[unit].b_errcnt = 0;
	setab[unit].b_actf = bp->b_actf;
#endif
	bp->b_resid = 0;
	biodone(bp);
	scsifree(&sc->sc_dq);
	if (setab[unit].b_actf)
		seustart(unit);
	else
		setab[unit].b_active = 0;
}

/*
 * Reset interface.
 */
int
se_reset(sc)
	struct se_softc *sc;
{
	int s;

	printf("se_reset()\n");

	sc->sc_flags |= SEF_RESET;
	/*
	 * Stop interface and re-initialize.
	 */
	se_stop(sc);
	se_init(sc);
}
 
/*
 * Take interface offline.
 */
void
se_stop(sc)
	struct se_softc *sc;
{
	int n = 5000;

	printf("se_stop()\n");
	/*
	 * Stop everything on the interface, and select page 0 registers.
	 */

	/*
	 * Wait for interface to enter stopped state, but limit # of checks
	 *	to 'n' (about 5ms). It shouldn't even take 5us on modern
	 *	DS8390's, but just in case it's an old one.
	 */
}

/*
 * Device timeout/watchdog routine. Entered if the device neglects to
 *	generate an interrupt after a transmit has been started on it.
 */
int
se_watchdog(ifp)
	struct ifnet *ifp;
{
	struct se_softc *sc = ifp->if_softc;

	log(LOG_ERR, "se%d: device timeout\n", unit);
	++sc->sc_arpcom.ac_if.if_oerrors;

	se_reset(sc);
}

/*
 * Initialize device. 
 */
void
se_init(sc)
	struct se_softc *sc;
{
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;
	int i, s;
	u_char	command;
	struct x68k_device *xd = sc->sc_xd;
	int unit = ifp->if_unit;
	register struct buf *bp = setab[xd->x68k_unit].b_actf;
	int ctlr, slave;
	int stat;

	printf("se_init()\n");

#if 0 /* NetBSD 1.1 ??? */
	/* address not known */
	if (ifp->if_addrlist == (struct ifaddr *)0)
		return;
#endif

	/* sefinish(unit, sc, bp);*/
	/*
	 * Initialize the NIC in the exact order outlined in the NS manual.
	 *	This init procedure is "mandatory"...don't change what or when
	 *	things happen.
	 */

	/* Reset transmitter flags. */
	sc->xmit_busy = 0;
	sc->sc_arpcom.ac_if.if_timer = 0;

	sc->txb_inuse = 0;
	sc->txb_next = 0;

	/*
	 * Set 'running' flag, and clear output active flag.
	 */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	sc->sc_flags &= ~SEF_RESET;
	/*
	 * ...and attempt to start output
	 */
	se_start(ifp);
}
 
/*
 * This routine actually starts the transmission on the interface
 */
static inline void se_xmit(ifp)
	struct ifnet *ifp;
{
	struct se_softc *sc = ifp->if_softc;
	u_short len = sc->txb_next_len;
	register struct buf *bp = malloc(sizeof *bp, M_DEVBUF, M_WAITOK);
	register struct scsi_fmt_cdb *cmd = &secmd[sc->sc_punit];
	struct x68k_device *xd = sc->sc_xd;
	int stat;
	int s;

	printf("se_xmit()\n");
	/*
	 * Set NIC for page 0 register access
	 */
	/*
	 * Set TX buffer start page
	 */
	sc->xmit_busy = 1;
	sc->sc_flags |= SEF_CMD;
	cmd->len = 6; /* all Ether+ commands are cdb6 */
	cmd->cdb[0] = CMD_SEND_ETHERPACKET;
	cmd->cdb[1] = 0x00; /* logical blocks */
	cmd->cdb[2] = 0x00;
	cmd->cdb[3] = len >> 8;
	cmd->cdb[4] = len & 0xff;
	cmd->cdb[5] = 0x00;
	sc->sc_cmd = cmd->cdb[0];

	bp->b_flags = B_BUSY|B_WRITE;
	bp->b_dev = sc->sc_punit;	/* XXX */
	bp->b_data = sc->txbuf;
	bp->b_bcount = len;
	bp->b_resid = len;
	bp->b_blkno = 0;
	bp->b_error = 0;
	ifp->if_timer = 4;

	sestrategy(bp);

	printf("se_xmit: waiting...\n");
	biowait(bp);
	printf("se_xmit: returned...\n");

}

/*
 * This routine actually starts the transmission on the interface
 */
static inline void se_rxmit(unit)
	int	unit;
{
	struct se_softc *sc = &se_softc[unit];
	register struct buf *bp = malloc(sizeof *bp, M_DEVBUF, M_WAITOK);
	register struct scsi_fmt_cdb *cmd = &secmd[sc->sc_punit];
	int stat;
	int s;

	printf("se_rxmit(%d)\n", unit);
	/*
	 * Set NIC for page 0 register access
	 */
	/*
	 * Set TX buffer start page
	 */
	sc->xmit_busy = 1;
	sc->sc_flags |= SEF_CMD;
	cmd->len = 6; /* all Ether+ commands are cdb6 */
	cmd->cdb[0] = CMD_RECIEVE_ETHERPACKET;
	cmd->cdb[1] = sc->sc_punit << 5; /* logical blocks and timeout*/
	cmd->cdb[2] = 0x00;
	cmd->cdb[3] = 0x06;
	cmd->cdb[4] = 0x0f;
	cmd->cdb[5] = 0x00;
	sc->sc_cmd = cmd->cdb[0];
	bzero(sc->rxbuf, (size_t)4);
#if 0
	s = splbio();
	while (bp->b_flags & B_BUSY) {
		if (bp->b_flags & B_DONE)
			break;
		bp->b_flags |= B_WANTED;
		sleep((caddr_t)bp, PRIBIO);
	}
	splx(s);
#endif
	bp->b_flags = B_BUSY|B_READ;
	bp->b_dev = sc->sc_punit;	/* XXX */
	bp->b_data = sc->rxbuf;
	bp->b_bcount = 1551;
	bp->b_resid = 1551;
	bp->b_blkno = 0;
	bp->b_error = 0;

	sestrategy(bp);
	sc->xmit_busy = 0;
}

/*
 * Start output on interface.
 * We make two assumptions here:
 *  1) that the current priority is set to splnet _before_ this code
 *     is called *and* is returned to the appropriate priority after
 *     return
 *  2) that the IFF_OACTIVE flag is checked before this code is called
 *     (i.e. that the output part of the interface is idle)
 */
int
se_start(ifp)
	struct ifnet *ifp;
{
	struct se_softc *sc = if->if_softc;
	int unit;
	int ctlr, slave;
	struct mbuf *m0, *m;
	caddr_t buffer;
	int len;
	u_short pktlen;
	int	s;

	printf("se_start()\n");
	if (sc->sc_flags & SEF_RESET) {
		printf("reset operation is now in progress.\n");
		return;
	}
	/* Don't transmit if interface is busy or not running */
	if ((sc->sc_arpcom.ac_if.if_flags & (IFF_RUNNING|IFF_OACTIVE)) != IFF_RUNNING)
		return 0;

	ctlr = sc->sc_xd->x68k_ctlr;
	slave = sc->sc_xd->x68k_slave;
	unit = sc->sc_punit;
outloop:
#if 0
	pktlen = sc->rxbuf[2] | (sc->rxbuf[3] << 8);
	if (pktlen > 4) {
		/*
		 * The byte count includes the FCS - Frame Check Sequence (a
		 *	32 bit CRC).
		 */
		if ((pktlen >= ETHER_MIN_LEN) && (pktlen <= ETHER_MAX_LEN)) {
			/*
			 * Go get packet. pktlen - 4 removes CRC from length.
			 * (packet_ptr + 1) points to data just after the packet ring
			 *	header (+4 bytes)
			 */
			se_get_packet(sc, (caddr_t)(sc->rxbuf), pktlen - 4);
			++sc->sc_arpcom.ac_if.if_ipackets;
		} else {
			/*
			 * Really BAD...probably indicates that the ring pointers
			 *	are corrupted. Also seen on early rev chips under
			 *	high load - the byte order of the length gets switched.
			 */
			log(LOG_ERR,
				"se%d: shared memory corrupt - invalid packet length %d\n",
				unit, pktlen);
			se_reset(sc);
			return;
		}
	}
	printf("get finished\n");
#endif
	/*
	 * See if there are buffered packets and an idle transmitter -
	 * should never happen at this point.
	 */
	printf("sc->txb_inuse == %d\n", sc->txb_inuse);
	if (sc->txb_inuse)
		if (sc->xmit_busy == 0) {
			printf("%s: packets buffers, but transmitter idle\n",
			    ifp->if_xname);
			se_xmit(ifp);
		} else {
			/* See if there is room to put another packet in the buffer. */
			printf("se_start: no room.\n");
			/* No room.  Indicate this to the outside world and exit. */
			ifp->if_flags |= IFF_OACTIVE;
			return;
		}

	IF_DEQUEUE(&sc->sc_arpcom.ac_if.if_snd, m);
	if (m == 0) {
	/*
	 * The following isn't pretty; we are using the !OACTIVE flag to
	 * indicate to the outside world that we can accept an additional
	 * packet rather than that the transmitter is _actually_
	 * active. Indeed, the transmitter may be active, but if we haven't
	 * filled the secondary buffer with data then we still want to
	 * accept more.
	 * Note that it isn't necessary to test the txb_inuse flag -
	 * we wouldn't have tried to de-queue the packet in the first place
	 * if it was set.
	 */
		printf("se_start: no packet?\n");
		ifp->if_flags &= ~IFF_OACTIVE;
		return;
	}

	/*
	 * Copy the mbuf chain into the transmit buffer
	 */
	m0 = m;

	buffer = sc->txbuf;

	for (len = 0; m != 0; m = m->m_next) {
		bbcopy(mtod(m, caddr_t), buffer, m->m_len);
		buffer += m->m_len;
       		len += m->m_len;
	}

	sc->txb_next_len = max(len, ETHER_MIN_LEN);
	printf("se_start: next length == %d\n", sc->txb_next_len);

	sc->txb_inuse++;

	if (sc->xmit_busy == 0){
		printf("se_start: no busy.\n");
		se_xmit(ifp);
	} else
		printf("se_start: busy.\n");

	/*
	 * If there is BPF support in the configuration, tap off here.
	 *   The following has support for converting trailer packets
	 *   back to normal.
	 */
#if NBPFILTER > 0
	if (sc->bpf) {
		u_short etype;
		int off, datasize, resid;
		struct ether_header *eh;
		struct trailer_header {
			u_short ether_type;
			u_short ether_residual;
		} trailer_header;
		char ether_packet[ETHER_MAX_LEN];
		char *ep;

		ep = ether_packet;

		/*
		 * We handle trailers below:
		 * Copy ether header first, then residual data,
		 * then data. Put all this in a temporary buffer
		 * 'ether_packet' and send off to bpf. Since the
		 * system has generated this packet, we assume
		 * that all of the offsets in the packet are
		 * correct; if they're not, the system will almost
		 * certainly crash in m_copydata.
		 * We make no assumptions about how the data is
		 * arranged in the mbuf chain (i.e. how much
		 * data is in each mbuf, if mbuf clusters are
		 * used, etc.), which is why we use m_copydata
		 * to get the ether header rather than assume
		 * that this is located in the first mbuf.
		 */
		/* copy ether header */
		m_copydata(m0, 0, sizeof(struct ether_header), ep);
		eh = (struct ether_header *) ep;
		ep += sizeof(struct ether_header);
		etype = ntohs(eh->ether_type);
		if (etype >= ETHERTYPE_TRAIL &&
		    etype < ETHERTYPE_TRAIL+ETHERTYPE_NTRAILER) {
			datasize = ((etype - ETHERTYPE_TRAIL) << 9);
			off = datasize + sizeof(struct ether_header);

			/* copy trailer_header into a data structure */
			m_copydata(m0, off, sizeof(struct trailer_header),
				&trailer_header.ether_type);

			/* copy residual data */
			m_copydata(m0, off+sizeof(struct trailer_header),
				resid = ntohs(trailer_header.ether_residual) -
				sizeof(struct trailer_header), ep);
			ep += resid;

			/* copy data */
			m_copydata(m0, sizeof(struct ether_header),
				datasize, ep);
			ep += datasize;

			/* restore original ether packet type */
			eh->ether_type = trailer_header.ether_type;

			bpf_tap(sc->bpf, ether_packet, ep - ether_packet);
		} else
			bpf_mtap(sc->bpf, m0);
	}
#endif
	printf("put finished\n");
	m_freem(m0);
	ifp->if_flags &= ~IFF_OACTIVE;
}
 
/*
 * Ethernet interface receiver interrupt.
 */
static inline void
se_rint(unit)
	int unit;
{
	register struct se_softc *sc = &se_softc[unit];
	u_char boundry, current;
	struct se_ring *packet_ptr;

	/* MOVED to se_start() */
}

/*
 * Ethernet interface interrupt processor
 */
void
seintr(unit, stat)
	register int unit;
	int stat;
{
	struct se_softc *sc = &se_softc[unit];
	register struct scsi_queue *dq = &sc->sc_dq;
	register struct scsi_xsense *xp = (struct scsi_xsense *)sesense[unit].sense;
	register struct buf *bp = setab[unit].b_actf;
	struct x68k_device *am = sc->sc_xd;
	int s;
	u_char	rxmit_flag = 0;
	u_char isr;
	int cond;

	printf("seintr(%d,%d)\n", unit, stat);
	/*
	 * clear watchdog timer
	 */
	sc->sc_arpcom.ac_if.if_timer = 0;

#ifdef DIAGNOSTIC
	if (bp == NULL) {
		printf("se%d: bp == NULL\n", unit);
		return;
	}
#endif

	if (stat == SCSI_IO_TIMEOUT) {
		printf("se%d: timeout error\n", unit);
	}

	if (am->x68k_dk >= 0)
		dk_busy &=~ (1 << am->x68k_dk);

	if (stat != 0) {
		if (stat > 0) {
#ifdef DEBUG
			if (sedebug & SEB_ERROR)
				printf("se%d: seintr: bad scsi status 0x%x\n",
					unit, stat);
#endif
			cond = seerror(unit, sc, am, stat);
		}
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
	} else {
		/* scsi command completed ok */
		printf("seintr: cdb0 = %d\n", dq->dq_cdb->cdb[0]);
		if(setab[unit].b_active == 1) {
			if ((dq->dq_cdb->cdb[0]) == 5) {
				printf("seintr: sent\n");
				++sc->sc_arpcom.ac_if.if_opackets;
				sc->sc_arpcom.ac_if.if_flags &= ~IFF_OACTIVE;
				--sc->txb_inuse;
				bp->b_resid = 0;
				
				/*
				 * Switch buffers if we are doing double-buffered transmits
				 */
				if ((sc->txb_next == 0) && (sc->txb_cnt > 1)) 
					sc->txb_next = 1;
				else
					sc->txb_next = 0;
				
				/*
				 * Set a timer just in case we never hear from the board again
				 */
				sc->xmit_busy = 0;

				rxmit_flag = 1;
			}
		}
	}
#ifdef DEBUG
	if ((sedebug & SE_BRESID) && bp->b_resid != 0)
		printf("b_resid %d b_flags 0x%x b_error 0x%x\n", 
		       bp->b_resid, bp->b_flags, bp->b_error);
#endif

	/*
	 * reset tx busy and output active flags
	 */
	sc->sc_arpcom.ac_if.if_flags &= ~IFF_OACTIVE;
	--sc ->txb_inuse;
	sefinish(unit, sc, bp);
	if (rxmit_flag == 1)
		se_rxmit(unit);
}
 
#ifdef __STDC__
secommand(int unit, u_int command, int cnt)
#else
secommand(unit, command, cnt)
	int unit;
	u_int command;
	int cnt;
#endif
{
	register struct se_softc *sc = &se_softc[unit];
	register struct buf *bp = malloc(sizeof *bp, M_DEVBUF, M_WAITOK);
	register struct scsi_fmt_cdb *cmd = &secmd[unit];
	register cmdcnt;
	int s;

	printf("secommand()\n");
	cmd->len = 6; /* all Ether+ commands are cdb6 */
	cmd->cdb[1] = sc->sc_punit;
	cmd->cdb[2] = cmd->cdb[3] = cmd->cdb[4] = cmd->cdb[5] = 0;
	cmdcnt = 0;

	/*
	 * XXX Assumption is that everything except Archive can take
	 * repeat count in cdb block.
	 */
	switch (command) {
	case EPCLEARTALLIES:
		cmd->cdb[0] = CMD_CLEAR_TALLIES;
		*(u_char *)(&cmd->cdb[1]) = 0x00;
		*(u_char *)(&cmd->cdb[2]) = 0x00;
		*(u_char *)(&cmd->cdb[3]) = 0x00;
		*(u_char *)(&cmd->cdb[4]) = 0x00;
		*(u_char *)(&cmd->cdb[5]) = 0x00;
		break;
	case EPSENDETHERPACKET:
		cmd->cdb[0] = CMD_SEND_ETHERPACKET;
		cmd->cdb[1] |= 0x00; /* logical blocks */
		*(u_char *)(&cmd->cdb[2]) = 0x00;
		*(u_char *)(&cmd->cdb[3]) = 0xff;	/* XXX */
		*(u_char *)(&cmd->cdb[4]) = 0xff;	/* XXX */
		*(u_char *)(&cmd->cdb[5]) = 0x00;
		break;
	case EPRECIEVEETHERPACKET:
		cmd->cdb[0] = CMD_RECIEVE_ETHERPACKET;
		cmd->cdb[1] |= 0x1f; /* logical blocks and timeout period(msb)*/
		*(u_char *)(&cmd->cdb[2]) = 0xff;	/* timeout period */
		*(u_char *)(&cmd->cdb[3]) = 0xff;	/* XXX */
		*(u_char *)(&cmd->cdb[4]) = 0xff;	/* XXX */
		*(u_char *)(&cmd->cdb[5]) = 0x00;
		break;
	case EPSETETHERADDR:
		cmd->cdb[0] = CMD_SET_ETHERADDR;
		cmd->cdb[1] |= 0x00; /* logical blocks */
		*(u_char *)(&cmd->cdb[2]) = 0x00;
		*(u_char *)(&cmd->cdb[3]) = 0x00;
		*(u_char *)(&cmd->cdb[4]) = 0x06;
		*(u_char *)(&cmd->cdb[5]) = 0x00;
		break;
	case EPSETMULTICASTADDR:
		cmd->cdb[0] = CMD_SET_MULTICASTADDR;
		cmd->cdb[1] |= 0x00; /* logical blocks */
		*(u_char *)(&cmd->cdb[2]) = 0x00;
		*(u_char *)(&cmd->cdb[3]) = 0x00;
		*(u_char *)(&cmd->cdb[4]) = 0x08;
		*(u_char *)(&cmd->cdb[5]) = 0x00;
		break;
	case EPSETPACKETRECEPTMODE:
		cmd->cdb[0] = CMD_SET_PACKETRECEPTMODE;
		cmd->cdb[1] |= 0x00; /* logical blocks */
		*(u_char *)(&cmd->cdb[2]) = 0xff;	/* XXX */
		*(u_char *)(&cmd->cdb[3]) = 0xff;	/* XXX */
		*(u_char *)(&cmd->cdb[4]) = 0xff;	/* XXX */
		*(u_char *)(&cmd->cdb[5]) = 0x00;
		break;
	case EPETHERNETRECIEVE:
		cmd->cdb[0] = CMD_ETHERNET_RECIEVE;
		cmd->cdb[1] |= 0x00; /* logical blocks */
		*(u_char *)(&cmd->cdb[2]) = 0x00;
		*(u_char *)(&cmd->cdb[3]) = 0x00;
		*(u_char *)(&cmd->cdb[4]) = 0x00;
		*(u_char *)(&cmd->cdb[5]) = 0x00;
		break;
	default:
		printf("se%d: secommand bad command 0x%x\n", 
		       unit, command);
	}

	sc->sc_flags |= SEF_CMD;
	sc->sc_cmd = cmd->cdb[0];

again:
#if 0
	s = splbio();
	while (bp->b_flags & B_BUSY) {
		if (bp->b_flags & B_DONE)
			break;
		bp->b_flags |= B_WANTED;
		sleep((caddr_t)bp, PRIBIO);
	}
	splx(s);
#endif
	bp->b_flags = B_BUSY|B_READ;
	bp->b_dev = (dev_t)unit;	/* XXX */
	bp->b_bcount = 0;
	bp->b_resid = 0;
	bp->b_blkno = 0;
	bp->b_error = 0;

	sestrategy(bp);

	biowait(bp);

	if (--cnt > 0)
		goto again;

	sc->sc_flags &= ~(SEF_CMD|SEF_WRTTN);
}

/*
 * Process an ioctl request. This code needs some work - it looks
 *	pretty ugly.
 */
int
se_ioctl(ifp, command, data)
	register struct ifnet *ifp;
	int command;
	caddr_t data;
{
	register struct ifaddr *ifa = (struct ifaddr *)data;
	struct se_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error = 0;

	printf("se_ioctl()\n");

	switch (command) {

	case SIOCSIFADDR:
		printf("se_ioctl:SIOCSIFADDR\n");
		ifp->if_flags |= IFF_UP;

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			se_init(sc);	/* before arpwhohas */
			/*
			 * See if another station has *our* IP address.
			 * i.e.: There is an address conflict! If a
			 * conflict exists, a message is sent to the
			 * console.
			 */
			((struct arpcom *)ifp)->ac_ipaddr =
				IA_SIN(ifa)->sin_addr;
			arpwhohas((struct arpcom *)ifp, &IA_SIN(ifa)->sin_addr);
			break;
#endif
#ifdef NS
		/*
		 * XXX - This code is probably wrong
		 */
		case AF_NS:
		    {
			register struct ns_addr *ina = &(IA_SNS(ifa)->sns_addr);

			if (ns_nullhost(*ina))
				ina->x_host =
					*(union ns_host *)(sc->sc_arpcom.ac_enaddr);
			else {
				/* 
				 * 
				 */
				bbcopy((caddr_t)ina->x_host.c_host,
				    (caddr_t)sc->sc_arpcom.ac_enaddr,
					sizeof(sc->sc_arpcom.ac_enaddr));
			}
			/*
			 * Set new address
			 */
			se_init(sc);
			break;
		    }
#endif
		default:
			se_init(sc);
			break;
		}
		break;

	case SIOCSIFFLAGS:
		printf("se_ioctl:SIOCSIFFLAGS\n");
		if (((ifp->if_flags & IFF_UP) == 0) &&
		    ((ifp->if_flags & IFF_RUNNING) != 0)) {
			/*
			 * If interface is marked down and it is running, then
			 * stop it.
			 */
			se_stop(sc);
			ifp->if_flags &= ~IFF_RUNNING;
		} else if ((ifp->if_flags & IFF_UP) != 0 &&
			   ((ifp->if_flags & IFF_RUNNING) == 0)) {
			/*
			 * If interface is marked up and it is stopped, then
			 * start it.
			 */
			se_init(sc);
		} else {
			/*
			 * Reset the interface to pick up changes in any other
			 * flags that affect hardware registers.
			 */
			se_stop(sc);
			se_init(sc);
		}
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		printf("se_ioctl:SIOC***MULTI\n");
		/* Update our multicast list. */
		error = (command == SIOCADDMULTI) ?
		    ether_addmulti(ifr, &sc->sc_arpcom) :
		    ether_delmulti(ifr, &sc->sc_arpcom);

		if (error == ENETRESET) {
			/*
			 * Multicast list has changed; set the hardware filter
			 * accordingly.
			 */
			se_stop(sc); /* XXX for ds_setmcaf? */
			se_init(sc);
			error = 0;
		}
		break;
	default:
		printf("se_ioctl:invalid IOCTL\n");
		error = EINVAL;
	}
	printf("IOCTL finished.\n");
	return (error);
}
 
/*
 * Macro to calculate a new address within shared memory when given an offset
 *	from an address, taking into account ring-wrap.
 */
#define	ringoffset(sc, start, off, type) \
	((type)( ((caddr_t)(start)+(off) >= (sc)->smem_end) ? \
		(((caddr_t)(start)+(off))) - (sc)->smem_end \
		+ (sc)->smem_ring: \
		((caddr_t)(start)+(off)) ))

/*
 * Retreive packet from shared memory and send to the next level up via
 *	ether_input(). If there is a BPF listener, give a copy to BPF, too.
 */
se_get_packet(sc, buf, len)
	struct se_softc *sc;
	char *buf;
	u_short len;
{
	struct ether_header *eh;
    	struct mbuf *m, *head, *se_ring_to_mbuf();
	u_short off;
	int resid;
	u_short etype;
	struct trailer_header {
		u_short	trail_type;
		u_short trail_residual;
	} trailer_header;

	/* Allocate a header mbuf */
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == 0)
		goto bad;
	m->m_pkthdr.rcvif = &sc->sc_arpcom.ac_if;
	m->m_pkthdr.len = len;
	m->m_len = 0;
	head = m;

	eh = (struct ether_header *)buf;

	/* The following sillines is to make NFS happy */
#define EROUND	((sizeof(struct ether_header) + 3) & ~3)
#define EOFF	(EROUND - sizeof(struct ether_header))

	/*
	 * The following assumes there is room for
	 * the ether header in the header mbuf
	 */
	head->m_data += EOFF;
	bbcopy(buf, mtod(head, caddr_t), sizeof(struct ether_header));
	buf += sizeof(struct ether_header);
	head->m_len += sizeof(struct ether_header);
	len -= sizeof(struct ether_header);

	etype = ntohs((u_short)eh->ether_type);

	/*
	 * Deal with trailer protocol:
	 * If trailer protocol, calculate the datasize as 'off',
	 * which is also the offset to the trailer header.
	 * Set resid to the amount of packet data following the
	 * trailer header.
	 * Finally, copy residual data into mbuf chain.
	 */
	if (etype >= ETHERTYPE_TRAIL &&
	    etype < ETHERTYPE_TRAIL+ETHERTYPE_NTRAILER) {

		off = (etype - ETHERTYPE_TRAIL) << 9;
		if ((off + sizeof(struct trailer_header)) > len)
			goto bad;	/* insanity */

		eh->ether_type = *ringoffset(sc, buf, off, u_short *);
		resid = ntohs(*ringoffset(sc, buf, off+2, u_short *));

		if ((off + resid) > len) goto bad;	/* insanity */

		resid -= sizeof(struct trailer_header);
		if (resid < 0) goto bad;	/* insanity */

		m = se_ring_to_mbuf(sc, ringoffset(sc, buf, off+4, char *), head, resid);
		if (m == 0) goto bad;

		len = off;
		head->m_pkthdr.len -= 4; /* subtract trailer header */
	}

	/*
	 * Pull packet off interface. Or if this was a trailer packet,
	 * the data portion is appended.
	 */
	m = se_ring_to_mbuf(sc, buf, m, len);
	if (m == 0) goto bad;

#if NBPFILTER > 0
	/*
	 * Check if there's a BPF listener on this interface.
	 * If so, hand off the raw packet to bpf. 
	 */
	if (sc->bpf) {
		bpf_mtap(sc->bpf, head);

		/*
		 * Note that the interface cannot be in promiscuous mode if
		 * there are no BPF listeners.  And if we are in promiscuous
		 * mode, we have to check if this packet is really ours.
		 *
		 * XXX This test does not support multicasts.
		 */
		if ((sc->sc_arpcom.ac_if.if_flags & IFF_PROMISC) &&
			bcmp(eh->ether_dhost, sc->sc_arpcom.ac_enaddr,
				sizeof(eh->ether_dhost)) != 0 &&
			bcmp(eh->ether_dhost, etherbroadcastaddr,
				sizeof(eh->ether_dhost)) != 0) {

			m_freem(head);
			return;
		}
	}
#endif

	(*sc->sc_arpcom.ac_if)(&sc->sc_arpcom.ac_if, head);
	return;

bad:	if (head)
		m_freem(head);
	return;
}

/*
 * Supporting routines
 */

/*
 * Given a source and destination address, copy 'amount' of a packet from
 *	the ring buffer into a linear destination buffer. Takes into account
 *	ring-wrap.
 */
static inline char *
se_ring_copy(sc,src,dst,amount)
	struct se_softc *sc;
	char	*src;
	char	*dst;
	u_short	amount;
{
	u_short	tmp_amount;

	bbcopy(src, dst, amount);

	return(src + amount);
}

/*
 * Copy data from receive buffer to end of mbuf chain
 * allocate additional mbufs as needed. return pointer
 * to last mbuf in chain.
 * sc = ed info (softc)
 * src = pointer in ed ring buffer
 * dst = pointer to last mbuf in mbuf chain to copy to
 * amount = amount of data to copy
 */
struct mbuf *
se_ring_to_mbuf(sc,src,dst,total_len)
	struct se_softc *sc;
	char *src;
	struct mbuf *dst;
	u_short total_len;
{
	register struct mbuf *m = dst;

	while (total_len) {
		register u_short amount = min(total_len, M_TRAILINGSPACE(m));

		if (amount == 0) { /* no more data in this mbuf, alloc another */
			/*
			 * If there is enough data for an mbuf cluster, attempt
			 * 	to allocate one of those, otherwise, a regular
			 *	mbuf will do.
			 * Note that a regular mbuf is always required, even if
			 *	we get a cluster - getting a cluster does not
			 *	allocate any mbufs, and one is needed to assign
			 *	the cluster to. The mbuf that has a cluster
			 *	extension can not be used to contain data - only
			 *	the cluster can contain data.
			 */ 
			dst = m;
			MGET(m, M_DONTWAIT, MT_DATA);
			if (m == 0)
				return (0);

			if (total_len >= MINCLSIZE)
				MCLGET(m, M_DONTWAIT);

			m->m_len = 0;
			dst->m_next = m;
			amount = min(total_len, M_TRAILINGSPACE(m));
		}

		src = se_ring_copy(sc, src, mtod(m, caddr_t) + m->m_len, amount);

		m->m_len += amount;
		total_len -= amount;

	}
	return (m);
}

/*
 * Compute the multicast address filter from the list of multicast addresses we
 * need to listen to.
 */
void
se_getmcaf(ac, af)
	struct arpcom *ac;
	u_long *af;
{
	struct ifnet *ifp = &ac->ac_if;
	struct ether_multi *enm;
	register u_char *cp, c;
	register u_long crc;
	register int i, len;
	struct ether_multistep step;

	/*
	 * Set up multicast address filter by passing all multicast addresses
	 * through a crc generator, and then using the high order 6 bits as an
	 * index into the 64 bit logical address filter.  The high order bit
	 * selects the word, while the rest of the bits select the bit within
	 * the word.
	 */

	if (ifp->if_flags & IFF_PROMISC) {
		ifp->if_flags |= IFF_ALLMULTI;
		af[0] = af[1] = 0xffffffff;
		return;
	}

	af[0] = af[1] = 0;
	ETHER_FIRST_MULTI(step, ac, enm);
	while (enm != NULL) {
		if (bcmp(enm->enm_addrlo, enm->enm_addrhi,
		    sizeof(enm->enm_addrlo)) != 0) {
			/*
			 * We must listen to a range of multicast addresses.
			 * For now, just accept all multicasts, rather than
			 * trying to set only those filter bits needed to match
			 * the range.  (At this time, the only use of address
			 * ranges is for IP multicast routing, for which the
			 * range is big enough to require all bits set.)
			 */
			ifp->if_flags |= IFF_ALLMULTI;
			af[0] = af[1] = 0xffffffff;
			return;
		}

		cp = enm->enm_addrlo;
		crc = 0xffffffff;
		for (len = sizeof(enm->enm_addrlo); --len >= 0;) {
			c = *cp++;
			for (i = 8; --i >= 0;) {
				if (((crc & 0x80000000) ? 1 : 0) ^ (c & 0x01)) {
					crc <<= 1;
					crc ^= 0x04c11db6 | 1;
				} else
					crc <<= 1;
				c >>= 1;
			}
		}
		/* Just want the 6 most significant bits. */
		crc >>= 26;

		/* Turn on the corresponding bit in the filter. */
		af[crc >> 5] |= 1 << ((crc & 0x1f) ^ 24);

		ETHER_NEXT_MULTI(step, enm);
	}
	ifp->if_flags &= ~IFF_ALLMULTI;
}
#endif
