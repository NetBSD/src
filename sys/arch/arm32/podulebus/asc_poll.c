/* $NetBSD: asc_poll.c,v 1.1 1996/01/31 23:25:32 mark Exp $ */

/*
 * Copyright (c) 1995 Mark Brinicombe.
 * Copyright (c) 1995 Brini.
 * All rights reserved.
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * asc.c
 *
 * Acorn SCSI card driver
 *
 * Created      : 01/07/95
 * Last updated : 01/07/95
 *
 *    $Id: asc_poll.c,v 1.1 1996/01/31 23:25:32 mark Exp $
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <scsi/scsi_all.h>
#include <scsi/scsiconf.h>
#include <machine/io.h>
#include <machine/irqhandler.h>
#include <machine/katelib.h>
#include <machine/psl.h>
#include <arm32/podulebus/podulebus.h>
#include <arm32/podulebus/ascreg.h>

#define ASC_DEBUG

#ifdef ASC_DEBUG
#define dprintf(x) { int s = spltty(); printf x ; (void)splx(s); }
int ascdebug = 1;
#else
#define dprintf(x)
#endif

#define MY_MANUFACTURER 0x00
#define MY_PODULE       0x02

#define SEVERELY_BROKEN	0x01

struct asc_softc {
	struct device	sc_device;
	int 		sc_podule_number;
	podule_t	*sc_podule;
	irqhandler_t 	sc_ih;
	u_int 		sc_pagereg;
	u_int 		sc_sbic_base;
	u_int 		sc_dmac_base;
	u_int 		sc_intstat;
	int		sc_state;
	struct scsi_link sc_link;      /* proto for sub devices */
	TAILQ_HEAD(,sbic_pending) sc_xslist;    /* LIFO */
	struct scsi_xfer *sc_xs;       /* transfer from high level code */
	int sc_timelimits[8];
};

int asc_scsi_cmd __P((struct scsi_xfer *));
void asc_minphys __P((struct buf *));

struct scsi_adapter asc_scsiswitch = {
	asc_scsi_cmd,		/* SCSI CMD */
	asc_minphys,		/* SCSI MINPHYS */
	0,			/* close target lun */
	0,			/* close target lun */
};
                                
struct scsi_device asc_scsidev = {
	NULL,           /* use default error handler */
	NULL,           /* do not have a start function */
	NULL,           /* have no async handler */
	NULL,           /* Use default done routine */
};

#define ASC_NSEG 4

/* Declare prototypes */

int ascprint	__P((void *, char *));

void init_dmac	__P((struct asc_softc *));
int ascreset	__P((struct asc_softc *));
int ascintr	__P((void *));
int asc_scsi_cdb	__P((struct asc_softc */*sc*/, char */*cdb*/, int /*cdb_size*/,
	int /*device_ID*/, u_char */*data_p*/, int */*p_txcnt*/, int /*dir*/,
	int /*lun*/));

int asc_asm_tx_data(int /*sbic_base*/, int /*txaddr*/, int /*txlen*/, int /*rw*/);

int asc_intwait	__P((struct asc_softc */*sc*/, int /*id*/));
int tc_wait	__P((struct asc_softc */*sc*/, int /*id*/));

/* I/O prototypes */

void read_sram	__P((u_int /*sramaddr*/, u_int /*len*/, u_int /*dest*/));
void write_sram	__P((u_int /*sramaddr*/, u_int /*len*/, u_int /*src*/));

/* Debugging prototypes */

void binary	__P((int));
void dump_dmac	__P((struct asc_softc *));
int dump_sbic	__P((struct asc_softc *));


static char message[256];
static char status[32];
static int debug = 0;
static int txdebug = 0;
static int error_code = 0;
static int data_count = 0;
static int mess_count = 0;
static int status_count = 0;

static int current_id;
static struct scsi_xfer *current_xs;
static int current_txlen;
static int current_txdata;

/*
 * int poduleprobe(struct device *parent, void *match, void *aux)
 *
 * Probe for the podule.
 */
 
int
ascprobe(parent, match, aux)
	struct device *parent;
	void *match;
	void *aux;
{
	struct podule_attach_args *pa = (void *)aux;
	int podule;

	podule = findpodule(MY_MANUFACTURER, MY_PODULE, pa->pa_podule_number);

/* Fail if we did not find it */

	if (podule == -1)
		return(0);

/* We found it */

	pa->pa_podule_number = podule;
	pa->pa_podule = &podules[podule];

	return(1);
}


/*
 * void poduleattach(struct device *parent, struct device *dev, void *aux)
 *
 * Attach podule.
 */
  
void
ascattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct asc_softc *sc = (void *)self;
	struct podule_attach_args *pa = (void *)aux;
	int error;
    
	sc->sc_podule_number = pa->pa_podule_number;
	if (sc->sc_podule_number == -1)
		panic("Podule has disappeared !");

	sc->sc_podule = &podules[sc->sc_podule_number];
	podules[sc->sc_podule_number].attached = 1;

	printf("\n");

	sc->sc_pagereg = sc->sc_podule->fast_base + ASC_PAGEREG;
	sc->sc_sbic_base = sc->sc_podule->mod_base + ASC_SBIC;
	sc->sc_dmac_base = sc->sc_podule->mod_base + ASC_DMAC;
        sc->sc_intstat = sc->sc_podule->fast_base + ASC_INTSTATUS;


/* Claiming a podule IRQ is something like this */

	sc->sc_ih.ih_func = ascintr;
	sc->sc_ih.ih_arg = sc;
	sc->sc_ih.ih_level = IPL_BIO;
	if (irq_claim(IRQ_PODULE /*IRQ_EXPCARD0 + sc->sc_podule_number*/, &sc->sc_ih))
		panic("Cannot install IRQ handler\n");

	sc->sc_link.adapter_softc = sc;
	sc->sc_link.adapter_target = 7;
	sc->sc_link.adapter = &asc_scsiswitch;
	sc->sc_link.device = &asc_scsidev;
	sc->sc_link.openings = 1;
	TAILQ_INIT(&sc->sc_xslist);

	sc->sc_state = 0;
	error = ascreset(sc);

	if (error != ASC_SUCCESS) {
		sc->sc_state = SEVERELY_BROKEN;
		printf("asc: reset failed err=%d - DRIVER IS DEAD\n", error);
	}

/*
 * attach all scsi units on us
 */

	config_found(self, &sc->sc_link, ascprint);
}


/*
 * print diag if pnp is NULL else just extra
 */

int
ascprint(auxp, pnp)
	void *auxp;
	char *pnp;
{
	if (pnp == NULL)
		return(UNCONF);
	return(QUIET);
}

                  
struct cfdriver asccd = {
	NULL, "asc", ascprobe, ascattach, DV_DULL, sizeof(struct asc_softc),
	    NULL, 0
};


int
asc_intwait(sc, id)
	struct asc_softc *sc;
	int id;
{
        int i = 0;
        int intr;

       	while (((intr = ReadByte(sc->sc_intstat)) & IS_SBIC_IRQ) == 0) {
		if (++i > sc->sc_timelimits[id]) {
			dprintf(("SBIC interrupt timed out %02x\n", intr));
			return(ASC_IRQ_TIMED_OUT);
		}
		DELAY(1);
	}
	return(ASC_SUCCESS);
}


int tc_wait(sc, id)
	struct asc_softc *sc;
	int id;
{
	u_int dmac_base = sc->sc_dmac_base;
	u_int intstat = sc->sc_intstat;
	u_int clearint = sc->sc_podule->fast_base + ASC_CLRINT;
	int i = 0;

	dprintf(("Waiting for DMAC interrupt\n"));

        while ((ReadByte(intstat) & IS_DMAC_IRQ) == 0) {
	        if (++i > sc->sc_timelimits[id]) {
			dprintf(("\nDMAC interrupt timed out\n"));
#ifdef ASC_DEBUG
			if (ascdebug) {
/*		               	dump_dmac(sc);*/
/*     		         	dump_sbic(sc);*/
			}
#endif
			return(ASC_DMA_TX_TIMED_OUT);
		}
	}

	WriteByte(clearint, 0); /* clear TC int on new board */

	WriteDMAC(DMAC_MASKREG, DMAC_SET_MASK); /* Set DMA mask */
	return(0);
}


void
init_dmac(sc)
	struct asc_softc *sc;
{
	u_int dmac_base = sc->sc_dmac_base;

	dprintf(("init_dmac:\n"));
	WriteDMAC(DMAC_INITIALISE, 0);
	WriteDMAC(DMAC_INITIALISE, DMAC_Bits);
	WriteDMAC(DMAC_CHANNEL, 0);
	WriteDMAC(DMAC_DEVCON1, DMAC_Ctrl1);
	WriteDMAC(DMAC_DEVCON2, DMAC_Ctrl2);
}


int
ascreset(sc)
	struct asc_softc *sc;
{
	u_int pagereg = sc->sc_pagereg;
	u_int sbic_base = sc->sc_sbic_base;
	u_int dmac_base = sc->sc_dmac_base;
	int loop;
	int error;

	dprintf(("ascreset:\n"));
/*	dprintf(("pagereg=%08x\n", (u_int) sc->sc_pagereg));
	dprintf(("sbic=%08x\n", (u_int) sc->sc_sbic_base));
	dprintf(("dmac=%08x\n", (u_int) sc->sc_dmac_base));
	dprintf(("adapter=%d\n", sc->sc_link.adapter_target));*/

/* Set up timeouts */

	for (loop = 0; loop < 8; ++loop)
		sc->sc_timelimits[loop] = 500000;

/* Reset the hardware */

	WriteByte(pagereg, 0x80);
	DELAY(500000);
	WriteByte(pagereg, 0x00);
	DELAY(250000);

	init_dmac(sc);

	WriteSBIC(SBIC_OWNID, sc->sc_link.adapter_target
	    | SBIC_ID_FS_8_10 | SBIC_ID_EAF);

	WriteSBIC(SBIC_COMMAND, SBIC_CMD_Reset);

/* Any responce ? */

	if (asc_intwait(sc, sc->sc_link.adapter_target) == ASC_SUCCESS) {
		dprintf(("aux status = %02x\n", ReadByte(sc->sc_sbic_base + SBIC_AUX_STATUS)));
		error = ReadSBIC(SBIC_SCSISTAT);
		if (error == SBIC_ResetOk || error == SBIC_ResetAFOk) {
			if (error == SBIC_ResetOk)
				printf("asc: WD33C93 advanced features not enabled\n");
			WriteSBIC(SBIC_CONTROL, SBIC_CTL_NO_DMA);	/* Burst DMA mode */
			WriteSBIC(SBIC_TIMEREG, 0x20);	/* timeout 250mS */
			WriteSBIC(SBIC_SYNCTX, 0x20);	/* min tx time */
			WriteSBIC(SBIC_TARGETLUN, 0x00);
			WriteSBIC(SBIC_SOURCEID, 0x20);
			return(ASC_SUCCESS);
		} else {
			printf("asc: WD33C39 reset status = %02x\n", error);
			return(ASC_CHECK_STATUS);
		}
	} else {
		return(ASC_IRQ_TIMED_OUT);
	}
}

/* Do a scsi command. */

struct scsi_xfer *cur_xs;

int
asc_scsi_cmd(xs)
	struct scsi_xfer *xs;
{
	struct scsi_link *sl = xs->sc_link;
	struct asc_softc *sc = (struct asc_softc *)sl->adapter_softc;
	int size;
	int err;

	if (sl->target == sl->adapter_target)
		return(ESCAPE_NOT_SUPPORTED);

	if (sc->sc_state == SEVERELY_BROKEN) {
		xs->error = XS_DRIVER_STUFFUP;
		scsi_done(xs);
		return(COMPLETE);
	}

	if (debug) {
		dprintf(("id=%d lun=%d cmdlen=%d datalen=%d opcode=%02x flags=%08x status=%02x retries=%02x\n",
		    sl->target, sl->lun, xs->cmdlen, xs->datalen, xs->cmd->opcode,
		    xs->flags, xs->status, xs->retries));
	}

	size = xs->datalen;
        err = asc_scsi_cdb(sc, (char *)xs->cmd, xs->cmdlen, sl->target, (u_char *)xs->data, &size, xs->flags, sl->lun);
	xs->resid = size;
	xs->status = 0;
	if (err == 0 && status_count > 0) {
		struct scsi_sense ss;
		int ss_err;
		int loop;

		xs->status = status[0];

		printf("ascstatus (%d) = ", status_count);
		for (loop = 0; loop < status_count; ++loop)
			printf("%02x ", status[status_count]);
		printf("\n");

		if (status[0] != 0) {		
			char *ptr = (char *)xs->cmd;
			u_int sbic_base = sc->sc_sbic_base;
			int loop;

			dprintf(("id=%d lun=%d cmdlen=%d datalen=%d opcode=%02x flags=%08x status=%02x retries=%02x\n",
			    sl->target, sl->lun, xs->cmdlen, xs->datalen, xs->cmd->opcode,
			    xs->flags, xs->status, xs->retries));

			for (loop = 0; loop < xs->cmdlen; ++loop)
				printf("%02x ", ptr[loop]);
			printf("\n");

			WriteByte(sbic_base + SBIC_ADDRREG, SBIC_CDB1TSECT);

		        for (loop = 0; loop < xs->cmdlen; loop++) {
				printf("%02x ", ReadByte(sbic_base + SBIC_DATAREG));
        		}
        		printf("\n");

		}


		switch (status[0] & 0xfe) { /* Bit 0 of status is reserved */
		case SCSI_OK : /* GOOD */
			break;
		case SCSI_CHECK : /* CHECK CONDITION */
			if (debug) {
				dprintf(("Getting sense"));
			}
			ss.opcode = REQUEST_SENSE;
			ss.byte2 = sl->lun << 5;
			ss.unused[0] = 0;
			ss.unused[1] = 0;
			ss.length = sizeof(struct scsi_sense_data);
			ss.control = 0;
		
			size = sizeof(struct scsi_sense_data);
			ss_err = asc_scsi_cdb(sc, (char *)&ss, sizeof(struct scsi_sense),
			    sl->target, (u_char *)&xs->sense, &size,
			    SCSI_DATA_IN, sl->lun);
			if (ss_err != 0) {
				dprintf(("Failed to get sense data err = %d stcount=%d %02x", ss_err, status_count, status[0]));
			} else {
				if (debug) {
					dprintf((" => %02x %02x\n", xs->sense.extended_flags,
					    xs->sense.extended_extra_bytes[3]));
				}
			}

			err = ASC_READ_SENSE;
			break;
		case SCSI_BUSY : /* BUSY */
			err = ASC_BUSY;
			break;
		default :
			dprintf(("Non zero status - %02x count=%d", status[0], status_count));
			err = ASC_READ_SENSE;
			break;
		}
	}
	if (debug)
		dprintf(("err = %02x resid=%d\n", err, xs->resid));
        switch (err) {
        case ASC_SUCCESS :
        	xs->error = XS_NOERROR;
        	break;
        case ASC_CHECK_STATUS :
        	xs->error = XS_DRIVER_STUFFUP;
        	break;
	case ASC_IRQ_TIMED_OUT :
		xs->error = XS_TIMEOUT;
		break;        	
	case ASC_UNEXPECTED_STATUS :
		xs->error = XS_TIMEOUT;
		break;        	
	case ASC_DMA_TX_TIMED_OUT :
		xs->error = XS_TIMEOUT;
		break;        	
	case ASC_SELECT_TIMED_OUT :
		xs->error = XS_TIMEOUT;
		break;        	
	case ASC_READ_SENSE:
		xs->error = XS_SENSE;
		break;
	case ASC_BUSY:
		xs->error = XS_BUSY;
		break;
        default:
        	xs->error = XS_DRIVER_STUFFUP;
        	break;
        }

	scsi_done(xs);

        return(COMPLETE);
}


void
asc_minphys(bp)
	struct buf *bp;
{
/*	if (bp->b_bcount > (ASC_NSEG * NBPG))
		bp->b_bcount = (ASC_NSEG * NBPG);*/
	minphys(bp);
}

int
ascintr(arg)
	void *arg;
{
	struct asc_softc *sc = (struct asc_softc *)arg;
	int intr;
	
	dprintf(("ascintr:"));
       	intr = ReadByte(sc->sc_intstat);
       	dprintf(("%02x\n", intr));

#if notyet
	if (!(intr & IS_IRQ_REQ)) {
		disable_irq(IRQ_PODULE);
		return(0);
	}
	if (intr & IS_SBIC_IRQ)
		asc_sbicintr(sc);
	if (intr & IS_DMAC_IRQ)
		asc_dmacintr(sc);
#endif

	disable_irq(IRQ_PODULE);
	return(0);
}


#if 0
/* I/O transfer routines. */

void
read_sram(sramaddr, len, dest)
	u_int sramaddr;
	u_int len;
	u_int dest;
{
	int loop;
	u_short *addr;

	addr = (u_short *)dest;
	for (loop = 0; loop < len; loop +=2 ) {
		*addr++ = ReadShort(sramaddr);
		sramaddr += 4;
	}
}


void
write_sram(sramaddr, len, src)
	u_int sramaddr;
	u_int len;
	u_int src;
{
	int loop;
	u_short *addr;

	addr = (u_short *)src;
	for (loop = 0; loop < len; loop +=2 ) {
		WriteShort(sramaddr, *addr++);
		sramaddr += 4;
	}
}



#ifdef ASC_DEBUG

/* Debugging functions */

void binary(i)
	int i;
{
	int j = 128;

	dprintf((" ="));
	while (j) {
		if (j & i) {
			dprintf((" 1"));
		} else {
			dprintf((" 0"));
		}
		j = j / 2;
	}
	dprintf(("\n"));
}

void dump_dmac(sc)
	struct asc_softc *sc;
{
	unsigned int dmac_base = sc->sc_podule->mod_base + ASC_DMAC;
	int sum;

	printf("\n\n");
	printf("\nContents of DMA Controller Registers :-\n");
	printf("Channel Select ---- ---- ---- BASE SEL3 SEL2 SEL1 SEL0");
	binary(ReadDMAC(DMAC_CHANNEL));
	printf("Device Control  AKL  RQL  EXW  ROT  CMP DDMA AHLD  MTM");
	binary(ReadDMAC(DMAC_DEVCON1));
	printf("Device Control ---- ---- ---- ---- ---- ----  WEV BHLD");
	binary(ReadDMAC(DMAC_DEVCON2));
	printf("Channel 0 Mode **TMODE** ADIR AUTI **TDIR*** ---- WORD");
	binary(ReadDMAC(DMAC_MODECON));
	printf("Status Register RQ3  RQ2  RQ1  RQ0  TC3  TC2  TC1  TC0");
	binary(ReadDMAC(DMAC_STATUS));
	printf("Request Register -- ---- ---- ---- SRQ3 SRQ2 SRQ1 SRQ0");
	binary(ReadDMAC(DMAC_REQREG));
	printf("Mask Register  ---- ---- ---- ----   M3   M2   M1   M0");
	binary(ReadDMAC(DMAC_MASKREG));

	WriteDMAC(DMAC_CHANNEL, 0); /* ch0 */
	sum = ReadDMAC(DMAC_TXCNTLO);
	sum = sum + ReadDMAC(DMAC_TXCNTHI)*256;
	printf("Channel 0 Transfer Count $%x \n", sum);
	sum = ReadDMAC(DMAC_TXADRLO);
	sum = sum + ReadDMAC(DMAC_TXADRMD)*256;
	sum = sum + ReadDMAC(DMAC_TXADRHI)*65536;
	printf("Channel 0 Transfer Address $%x \n", sum);

	WriteDMAC(DMAC_CHANNEL, 1); /* ch0 */
	sum = ReadDMAC(DMAC_TXCNTLO);
	sum = sum + ReadDMAC(DMAC_TXCNTHI)*256;
	printf("Channel 1 Transfer Count $%x \n", sum);
	sum = ReadDMAC(DMAC_TXADRLO);
	sum = sum + ReadDMAC(DMAC_TXADRMD)*256;
	sum = sum + ReadDMAC(DMAC_TXADRHI)*65536;
	printf("Channel 1 Transfer Address $%x \n", sum);
}


int dump_sbic(sc)
	struct asc_softc *sc;
{
	u_int sbic_base = sc->sc_sbic_base;
	int i, j;

	printf("\n\n");
	printf("\nAuxiliary status INT  LCI  BSY  CIP    0    0   PE  DBR");
	binary(ReadByte(sbic_base + SBIC_AUX_STATUS));
	printf("Own ID           FS1  FS0    0  EHP  EAF  ID2  ID1  ID0");
	binary(ReadSBIC(SBIC_OWNID));
	printf("Control register DM2  DM1  DM0  HHP  EDI  IDI   HA  HSP");
	binary(ReadSBIC(SBIC_CONTROL));
	printf("Target LUN       TLV  DOK    0    0    0  TL2  TL1  TL0");
	binary(ReadSBIC(SBIC_TARGETLUN));
	printf("Synchronous Tx     0  TP2  TP1  TP0  OF3  OF2  OF1  OF0");
	binary(ReadSBIC(SBIC_SYNCTX));
	printf("Destination ID   SCC  DPD    0    0    0  DI2  DI1  DI0");
	binary(ReadSBIC(SBIC_DESTID));
	printf("Source ID         ER   ES  DSP    0  SIV  SI2  SI1  SI0");
	binary(ReadSBIC(SBIC_SOURCEID));
	printf("SCSI status     **Interrupt type***  **Int. qualifier**");
	j = ReadSBIC(SBIC_SCSISTAT); binary(j);
	printf("Timeout period");
	binary(ReadSBIC(SBIC_TIMEREG));
	printf("Command Phase ");
	binary(ReadSBIC(SBIC_COMPHASE));
	printf("Command code   = %x\n", ReadSBIC(SBIC_COMMAND));

	i = WriteByte(sbic_base + SBIC_ADDRREG, SBIC_CDB1TSECT); /* pre-select reg. */
	printf("CDB contents:-\n");
	for (i = 0; i < 6; ++i) {
		printf("%2d ", i);
		binary(ReadByte(sbic_base + SBIC_DATAREG));
	}

	i = WriteByte(sbic_base + SBIC_ADDRREG, SBIC_TXCOUNT1); /* pre-select reg. */
	i = ReadByte(sbic_base + SBIC_DATAREG) * 0x10000;
	i = i + ReadByte(sbic_base + SBIC_DATAREG) * 0x100;
	i = i + ReadByte(sbic_base + SBIC_DATAREG);
	printf("Transfer count = $%x\n", i);

	return(j);
}

#endif
#endif

int asc_tx_data(sc, txaddr, txlen, rw, device_ID)
	struct asc_softc *sc;
	int txaddr; /* address in main memory to read/write data */
	int txlen; /* transfer length in bytes */
	int rw; /* 0 = read from target, 1 = write to target */
	int device_ID;
/* break transfer up into 'blklen' blocks and check interrupt between blocks */
{
	u_int sbic_base = sc->sc_sbic_base;
        int byte;
        int i;
        int status;

	if (rw & SCSI_DATA_OUT) {
                WriteSBIC(SBIC_COMMAND, SBIC_Sel_tx_wATN); /* initiate command */

		if (txdebug)
			dprintf(("Writing %d bytes\n", txlen));

		while (txlen > 0) {
			do {
				status = ReadByte(sbic_base + SBIC_AUX_STATUS);
			} while ((status & 1) == 0 && (status & 128) == 0);
			if (status & 128) break;
			byte = *((u_char *)txaddr);
			WriteSBIC(SBIC_DATA, byte);
			++txaddr;
			--txlen;
		}
		if (txdebug)
			dprintf(("tx left = %d\n", txlen));
		return(txlen);
	}
	if (rw & SCSI_DATA_IN) {          /* data transfer in, ie read */
		WriteSBIC(SBIC_COMMAND, SBIC_Sel_tx_wATN); /* initiate command */

		if (txdebug)
			dprintf(("Reading %d bytes ... ", txlen));

		while (txlen > 0) {
			do {
				status = ReadByte(sbic_base + SBIC_AUX_STATUS);
			} while ((status & 1) == 0 && (status & 128) == 0);
			if (status & 128) break;
			byte = ReadSBIC(SBIC_DATA);
			*((u_char *)txaddr) = byte;
			++txaddr;
			--txlen;
		}
		if (txdebug)
			dprintf(("tx left = %d\n", txlen));
	        return(txlen);
	}
        return(txlen);
}


int end_of_com(sc, device_ID, data_p, p_txcnt)
	struct asc_softc *sc;
	int device_ID; /* current device number */
	u_char *data_p; /* current data pointer */
	int *p_txcnt;
{
	u_int sbic_base = sc->sc_sbic_base;
        int i;
        int end_found = 0;
        char *mess_p = message;
        char *status_p = status;
        int phase;
	int aux;
	int dummy;
	int ddone = 0;
	int dst = 0;

	if (p_txcnt && *p_txcnt > 0) {
		printf("p_txcnt=%d\n", *p_txcnt);
		debug = 1;
		dst = 1;
	} else debug = 0;
		
	do {
	        error_code = asc_intwait(sc, device_ID);

	        if ((error_code == ASC_IRQ_TIMED_OUT) & debug) {
	        	dprintf(("IRQ timedout\n"));
	        	dprintf(("Aborting "));
			WriteSBIC(SBIC_COMMAND, SBIC_Abort); /* Initiate command */
		        error_code = asc_intwait(sc, device_ID);
	        	dprintf((" - %02x\n", error_code));
	        	return(ASC_IRQ_TIMED_OUT);
	        }
 
 		aux = ReadByte(sbic_base + SBIC_AUX_STATUS);
                i = ReadSBIC(SBIC_SCSISTAT);  /* read SCSI status */
                phase = ReadSBIC(SBIC_COMPHASE);  /* read SCSI command phase*/

                if (debug && !(i == 0x19 && ddone != 0))
                	dprintf(("SCSI status for switch is %02x cp=%02x aux=%02x\n", i, phase, aux));

                switch(i) {

		case 0x42:
			if (debug)
				dprintf(("Select timeout - Disconnected\n"));
			error_code = ASC_SELECT_TIMED_OUT;
			end_found = 1;
			break;

		case 0x85 :
			if (debug)
				dprintf(("Disconnected\n"));
			end_found = 1;
			break;

                case 0x16: /* Select and transfer completed OK */
                	if (debug)
                		dprintf(("Select and transfer complete\n"));
                        break;

                case 0x49: /* unexpected data in */
			if (debug)
				dprintf(("Unexpected data in phase\n"));
                        WriteSBIC(SBIC_COMMAND, 0xa0); /* transfer info, single byte */
                        while (!(ReadByte(sbic_base + SBIC_AUX_STATUS) & 1)); /* wait for DBR set */
                        if (*p_txcnt > 0) {
	                        *data_p++ = ReadSBIC(SBIC_DATA); /* read out a byte */
	                        *p_txcnt -= 1;
	               	} else {
	               		dummy = ReadSBIC(SBIC_DATA);
	               	}
                        break;

                case 0x19: /* expected data in */
			if (debug && ddone == 0)
				dprintf(("Data in phase requested\n"));
                        WriteSBIC(SBIC_COMMAND, 0xa0); /* transfer info, single byte */
                        while (!(ReadByte(sbic_base + SBIC_AUX_STATUS) & 1)); /* wait for DBR set */
                        if (*p_txcnt > 0) {
	                        *data_p++ = ReadSBIC(SBIC_DATA); /* read out a byte */
	                        *p_txcnt -= 1;
	               	} else {
	               		dummy = ReadSBIC(SBIC_DATA);
	               	}
	               	ddone = 1;
                        break;

                case 0x48: /* unexpected data out */
			if (debug)
				dprintf(("Unexpected data out phase\n"));
                        WriteSBIC(SBIC_COMMAND, 0xa0); /* transfer info, single byte */
                        while (!(ReadByte(sbic_base + SBIC_AUX_STATUS) & 1)); /* wait for DBR set */
                        if (*p_txcnt > 0) {
	                        WriteSBIC(SBIC_DATA, *data_p++);
	                        *p_txcnt -= 1;
	               	} else {
	                        WriteSBIC(SBIC_DATA, 0);
	               	}
                        break;
                case 0x18: /* expected data out */
			if (debug)
				dprintf(("Data out phase requested\n"));
                        WriteSBIC(SBIC_COMMAND, 0xa0); /* transfer info, single byte */
                        while (!(ReadByte(sbic_base + SBIC_AUX_STATUS) & 1)); /* wait for DBR set */
                        if (*p_txcnt > 0) {
	                        WriteSBIC(SBIC_DATA, *data_p++);
	                        *p_txcnt -= 1;
	               	} else {
	                        WriteSBIC(SBIC_DATA, 0);
	               	}
                        break;

                case 0x4b: /* unexpected status in */
			if (debug)
	                	dprintf(("unexpected status in\n"));
                        WriteSBIC(SBIC_COMMAND, 0xa0); /* transfer info, single byte */
                        while (!(ReadByte(sbic_base + SBIC_AUX_STATUS) & 1)); /* wait for DBR set */
                        *status_p++ = ReadSBIC(SBIC_DATA); /* read out a byte */
                        if (debug) dprintf(("Message or status byte was %x\n", *(status_p-1)));
                        status_count++;
                        if (status_count > 32) status_p = status;
                        break;

                case 0x1b: /* status in */
                	if (debug)
                		dprintf(("Status in\n"));
                        WriteSBIC(SBIC_COMMAND, 0xa0); /* transfer info, single byte */
                        while (!(ReadByte(sbic_base + SBIC_AUX_STATUS) & 1)); /* wait for DBR set */
                        *status_p++ = ReadSBIC(SBIC_DATA); /* read out a byte */
                        if (debug) dprintf(("Message or status byte was %x\n", *(status_p-1)));
                        status_count++;
                        if (status_count > 255) status_p = status;
                        break;

                case 0x1f: /* message in */
                	if (debug)
                		dprintf(("Message in\n"));
                        WriteSBIC(SBIC_COMMAND, 0xa0); /* transfer info, single byte */
                        while (!(ReadByte(sbic_base + SBIC_AUX_STATUS) & 1)); /* wait for DBR set */
                        *mess_p++ = ReadSBIC(SBIC_DATA); /* read out a byte */
                        if (debug) dprintf(("Message or status byte was %x\n", *(mess_p-1)));
                        mess_count++;
                        if (mess_count > 255) mess_p = message;
                        break;


                case 0x20: /* message in paused */
                	if (debug)
                		dprintf(("Message in paused\n"));
                        WriteSBIC(SBIC_COMMAND, 0x03); /* negate ACK */
                        break;

                default:
                        error_code = ASC_UNEXPECTED_STATUS;
                        break;
                }
        } while (!end_found);

	if (ReadByte(sbic_base + SBIC_AUX_STATUS) & 0x80) {
		if (debug)
			dprintf(("Interrupt still pending\n"));
		end_of_com(sc, device_ID, data_p, p_txcnt);
	}

        return(error_code);

}


int asc_scsi_cdb(sc, cdb, cdb_size, device_ID, data_p, p_txcnt, dir, lun)
	struct asc_softc *sc;
	char *cdb; /* pointer to cdb array of bytes */
	int cdb_size; /* number of bytes in the cdb 6, 10 or 12 */
	int device_ID; /* target ID, range 0 to 6 */
	u_char *data_p; /* pointer to data */
	int *p_txcnt; /* pointer to number of bytes to transfer */
	int dir; /* direction of transfer, 0=read from peripheral, 1=write to it */
	int lun;
{
	u_int sbic_base = sc->sc_sbic_base;
        int i;
        int items = *p_txcnt;

	status_count = 0;
	mess_count = 0;

	if (ReadByte(sbic_base + SBIC_AUX_STATUS) & 0x10)
		dprintf(("Command in progress\n"));
	if (ReadByte(sbic_base + SBIC_AUX_STATUS) & 0x20)
		dprintf(("Busy\n"));
	if (ReadByte(sbic_base + SBIC_AUX_STATUS) & 0x40)
		if (debug)
			dprintf(("Last command ignored\n"));
	if (ReadByte(sbic_base + SBIC_AUX_STATUS) & 0x80) {
		dprintf(("Interrupt pending\n"));
		end_of_com(sc, device_ID, data_p);
	}

	if (dir & SCSI_DATA_IN) {
		WriteSBIC(SBIC_DESTID, device_ID | SBIC_DID_DPD);	/* set target ID */
	} else {
		WriteSBIC(SBIC_DESTID, device_ID);	/* set target ID */
	}

	WriteSBIC(SBIC_TARGETLUN, lun);

/* Preselect the CDB register */
  
	WriteByte(sbic_base + SBIC_ADDRREG, SBIC_CDB1TSECT);

        for (i = 0; i < cdb_size; i++) {
                /* binary(cdb[i]); */
		WriteByte(sbic_base + SBIC_DATAREG, cdb[i]);
        }
        
        /* set SBIC transfer count */

	WriteSBIC(SBIC_TXCOUNT1, (items >> 16) & 0xff);
	WriteSBIC(SBIC_TXCOUNT2, (items >> 8) & 0xff);
	WriteSBIC(SBIC_TXCOUNT3, items & 0xff);

        if (data_p) {
/*                *p_txcnt = asc_tx_data(sc, (int)data_p, items, dir, device_ID);*/
		*p_txcnt = asc_asm_tx_data(sc->sc_sbic_base, (int)data_p, items, dir);
	} else {
		WriteSBIC(SBIC_COMMAND, SBIC_Sel_tx_wATN); /* Initiate command */
	}

        i = end_of_com(sc, device_ID, data_p, p_txcnt);
        return(i);
}

/* End of asc.c */
