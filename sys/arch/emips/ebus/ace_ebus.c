/*	$NetBSD: ace_ebus.c,v 1.4.6.3 2014/08/20 00:02:51 tls Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was written by Alessandro Forin and Neil Pittman
 * at Microsoft Research and contributed to The NetBSD Foundation
 * by Microsoft Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ace_ebus.c,v 1.4.6.3 2014/08/20 00:02:51 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/syslog.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/kthread.h>
#include <sys/lock.h>
#include <sys/queue.h>

#include <sys/rnd.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include "locators.h"
#include <prop/proplib.h>

#include <emips/ebus/ebusvar.h>
#include <emips/emips/machdep.h>
#include <machine/emipsreg.h>

/* Structure returned by the Identify command (see CFlash specs)
 * NB: We only care for the first sector so that is what we define here.
 * NB: Beware of mis-alignment for all 32bit things
 */
typedef struct _CFLASH_IDENTIFY {
	uint16_t Signature;				/* Word 0 */
#define CFLASH_SIGNATURE 0x848a
	uint16_t DefaultNumberOfCylinders;		/* Word 1 */
	uint16_t Reserved1;				/* Word 2 */
	uint16_t DefaultNumberOfHeads;			/* Word 3 */
	uint16_t Obsolete1[2];				/* Word 4 */
	uint16_t DefaultSectorsPerTrack;		/* Word 6 */
	uint16_t SectorsPerCard[2];			/* Word 7 */
	uint16_t Obsolete2;				/* Word 9 */
	uint8_t  SerialNumber[20]; /* padded, right-justified Word 10 */
	uint16_t Obsolete3[2];				/* Word 20 */
	uint16_t EccBytesInRWLong;			/* Word 22 */
	uint8_t  FirmwareRevision[8];			/* Word 23 */
	uint8_t  ModelNumber[40];			/* Word 27 */
	uint16_t SectorsInRWMultiple;			/* Word 47 */
	uint16_t Reserved2;				/* Word 48 */
	uint16_t Capabilities;				/* Word 49 */
	uint16_t Reserved3;				/* Word 50 */
	uint16_t PioMode;				/* Word 51 */
	uint16_t Obsolete4;				/* Word 52 */
	uint16_t FieldValidity;				/* Word 53 */
	uint16_t CurrentNumberOfCylinders;		/* Word 54 */
	uint16_t CurrentNumberOfHeads;			/* Word 55 */
	uint16_t CurrentSectorsPerTrack;		/* Word 56 */
	uint16_t CurrentCapacity[2];			/* Word 57 */
	uint16_t MultiSectorSettings;			/* Word 59 */
	uint16_t NumberOfAddressableSectors[2];		/* Word 60 */
	uint16_t Reserved4;				/* Word 62 */
	uint16_t MultiWordDmaTransfer;			/* Word 63 */
	uint16_t AdvancedPioModes;			/* Word 64 */
	uint16_t MinimumMultiWordDmaTiming;		/* Word 65 */
	uint16_t RecommendedMultiWordDmaTiming;		/* Word 66 */
	uint16_t PioTimingNoFlowControl;		/* Word 67 */
	uint16_t PioTimingWithFlowControl;		/* Word 68 */
	uint16_t Reserved5[13];				/* Word 69 */
	uint16_t FeaturesSupported[3];			/* Word 82 */
	uint16_t FeaturesEnabled[3];			/* Word 85 */
	uint16_t UdmaMode;				/* Word 88 */
	uint16_t SecurityEraseTime;			/* Word 89 */
	uint16_t EnhancedSecurityEraseTime;		/* Word 90 */
	uint16_t CurrentPowerManagementValue;		/* Word 91 */
	uint8_t  Reserved6[72];				/* Word 92-127 */
	uint8_t  SecondHalf[256];			/* Word 128-255 */
} CFLASH_IDENTIFY, *PCFLASH_IDENTIFY;

#define SIZEOF_IDENTIFY CF_SECTOR_SIZE /* must be a sector multiple */

/* Instead of dragging in atavar.h.. */
/*
 * Parameters/state needed by the controller to perform an ATA bio.
 */
struct ace_bio {
	volatile int flags;/* cmd flags */
#define	ATA_POLL	0x0002	/* poll for completion */
#define	ATA_SINGLE	0x0008	/* transfer must be done in singlesector mode */
#define	ATA_READ	0x0020	/* transfer is a read (otherwise a write) */
#define	ATA_CORR	0x0040	/* transfer had a corrected error */
	daddr_t		blkno;	/* block addr */
	daddr_t		blkdone;/* number of blks transferred */
	size_t		nblks;	/* number of blocks currently transferring */
	size_t		nbytes;	/* number of bytes currently transferring */
	char		*databuf;/* data buffer address */
	volatile int	error;
#define	NOERROR		0	/* There was no error (r_error invalid),
				   else see acedone()*/
#define FAILED(er) (er != 0)
#define EDOOFUS EIO

	uint32_t	r_error;/* copy of status register */
#ifdef HAS_BAD144_HANDLING
	daddr_t		badsect[127];/* 126 plus trailing -1 marker */
#endif
};
/* End of atavar.h*/

struct ace_softc {
	/* General disk infos */
	device_t sc_dev;

	struct disk sc_dk;
	struct bufq_state *sc_q;
	struct callout sc_restart_ch;

	/* IDE disk soft states */
	struct buf *sc_bp; /* buf being transfered */
	struct buf *active_xfer; /* buf handoff to thread  */
	/* current transfer data */
	struct ace_bio sc_bio; /* current transfer */

	struct proc *ch_thread;
	int ch_flags;
#define ATACH_SHUTDOWN 0x02        /* thread is shutting down */
#define ATACH_IRQ_WAIT 0x10        /* thread is waiting for irq */
#define ATACH_DISABLED 0x80        /* channel is disabled */
#define ATACH_TH_RUN   0x100       /* the kernel thread is working */
#define ATACH_TH_RESET 0x200       /* someone ask the thread to reset */

	int openings;
	int media_has_changed;
#define    ACECE_MC    0x20    /* media changed */
#define    ACECE_MCR   0x08    /* media change requested */
	struct _CFLASH_IDENTIFY sc_params;/* drive characteristics found */

	int sc_flags;
#define	ACEF_WLABEL	0x004 /* label is writable */
#define	ACEF_LABELLING	0x008 /* writing label */
#define ACEF_LOADED	0x010 /* parameters loaded */
#define ACEF_WAIT	0x020 /* waiting for resources */
#define ACEF_KLABEL	0x080 /* retain label after 'full' close */

	uint64_t sc_capacity;
	uint32_t sc_multi; /* max sectors per xfer */

	struct	_Sac   *sc_dr;		/* reg pointers */
	int hw_busy;
	int retries; /* number of xfer retry */

	krndsource_t	rnd_source;
};

int  ace_ebus_match(device_t, cfdata_t, void *);
void ace_ebus_attach(device_t, device_t, void *);
void aceattach(struct ace_softc *);
int	 acedetach(device_t, int);
int	 aceactivate(device_t, enum devact);

void  acedone(struct ace_softc *);
static void ace_set_geometry(struct ace_softc *ace);

CFATTACH_DECL_NEW(ace_ebus, sizeof(struct ace_softc),
    ace_ebus_match, ace_ebus_attach, acedetach, aceactivate);

int  ace_ebus_intr(void *cookie, void *f);

static void sysace_thread(void *arg);

int
ace_ebus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct ebus_attach_args *d = aux;
	struct _Sac *sac = (struct _Sac *)d->ia_vaddr;

	if (strcmp("ace", d->ia_name) != 0)
		return 0;
	if ((sac == NULL) ||
	    ((sac->Tag & SAC_TAG) != PMTTAG_SYSTEM_ACE))
		return 0;
	return 1;
}

void
ace_ebus_attach(device_t parent, device_t self, void *aux)
{
	struct ace_softc *ace = device_private(self);
	struct ebus_attach_args *ia = aux;
	int error;

	ace->sc_dev = self;

	/*
	 * It's on the baseboard, with a dedicated interrupt line.
	 */
	ace->sc_dr = (struct _Sac *)ia->ia_vaddr;
#if DEBUG
	printf(" virt=%p", (void*)ace->sc_dr);
#endif
	printf(" : System ACE\n");

	ebus_intr_establish(parent, (void*)ia->ia_cookie, IPL_BIO,
	    ace_ebus_intr, ace);

	config_pending_incr(self);

	error = kthread_create(PRI_NONE, 0, NULL, sysace_thread,
	    ace, NULL, "%s", device_xname(ace->sc_dev));
	if (error)
		aprint_error_dev(ace->sc_dev, "unable to create kernel "
		    "thread: error %d\n", error);
}

/*
 * Sysace driver I(af) wrote for FreeBsd.
 */
#define CF_SECBITS     9
#define CF_SECTOR_SIZE (1 << CF_SECBITS)

static int sysace_attach(struct ace_softc *sc);
static int sysace_reset(struct ace_softc *sc);
static int sysace_identify(struct ace_softc *sc);
static int sysace_lock_registers(struct ace_softc *sc);
static int sysace_unlock_registers(struct ace_softc *sc);
static int sysace_start(struct ace_softc *sc, uint32_t Command, uint32_t Lba,
			uint32_t nSectors);
static int sysace_validate(struct ace_softc *sc, daddr_t start, size_t *pSize);
static int sysace_read_at (struct ace_softc *sc, daddr_t start_sector,
			   char *buffer, size_t nblocks, size_t * pSizeRead);
static int sysace_write_at(struct ace_softc *sc, daddr_t start_sector,
			   char *buffer, size_t nblocks, size_t * pSizeWritten);
#ifdef USE_ACE_FOR_RECONFIG /* Old code, despised and replaced by ICAP */
static int sysace_send_config(struct ace_softc *sc,
                              uint32_t *Data, unsigned int nBytes);
#endif

#define DEBUG_INTR   0x01
#define DEBUG_XFERS  0x02
#define DEBUG_STATUS 0x04
#define DEBUG_FUNCS  0x08
#define DEBUG_PROBE  0x10
#define DEBUG_WRITES 0x20
#define DEBUG_READS  0x40
#define DEBUG_ERRORS 0x80
#ifdef DEBUG
int ace_debug = DEBUG_ERRORS /*|DEBUG_WRITES*/;
#define ACE_DEBUG(x) (ace_debug & (x))
#define DBGME(_lev_,_x_) if ((_lev_) & ace_debug) _x_
#else
#define ACE_DEBUG(x) (0)
#define DBGME(_lev_,_x_)
#endif
#define DEBUG_PRINT(_args_,_lev_) DBGME(_lev_,printf _args_)

static int
sysace_attach(struct ace_softc *sc)
{
	int error;

	DBGME(DEBUG_FUNCS, printf("Sysace::delayed_attach %p\n", sc));

	sc->media_has_changed = TRUE;
	sc->sc_capacity = 0;

	error = sysace_reset(sc);
	if (error) {
		device_printf(sc->sc_dev,
		    "failed to reset, errno=%d\n", error);
		goto Out;
	}

	error = sysace_identify(sc);
	if (error) {
		device_printf(sc->sc_dev,
		    "failed to identify card, errno=%d.\n", error);
		goto Out;
	}

	DBGME(DEBUG_PROBE, device_printf(sc->sc_dev,
	    "Card has %qx sectors.\n", sc->sc_capacity));
	if (sc->sc_capacity == 0) {
		device_printf(sc->sc_dev, "size 0, no card? Wont work.\n");
		error = EDOOFUS;
		goto Out;
	}

	sc->media_has_changed = FALSE;
Out:
	return error;
}

static void
sysace_wedges(void *arg);
extern int	dkwedge_autodiscover;

/*
 * Aux temp thread to avoid deadlock when doing
 * the partitio.. ahem wedges thing.
 */
static void
sysace_wedges(void *arg)
{
	struct ace_softc *sc = arg;

	DBGME(DEBUG_STATUS, printf("Sysace::wedges started for %p\n", sc));

	/* Discover wedges on this disk. */
	dkwedge_autodiscover = 1;
	dkwedge_discover(&sc->sc_dk);

	config_pending_decr(sc->sc_dev);

	DBGME(DEBUG_STATUS, printf("Sysace::thread done for %p\n", sc));
	kthread_exit(0);
}

static void
sysace_thread(void *arg)
{
	struct ace_softc *sc = arg;
	struct buf *bp;
	int s, error;

	DBGME(DEBUG_STATUS, printf("Sysace::thread started for %p\n", sc));

	s = splbio();
	aceattach(sc);
	splx(s);

	error = kthread_create(PRI_NONE, 0 /* MPSAFE??? */, NULL,
	    sysace_wedges, sc, NULL, "%s.wedges", device_xname(sc->sc_dev));
	if (error)
		aprint_error_dev(sc->sc_dev, "wedges: unable to create "
		    "kernel thread: error %d\n", error);

	DBGME(DEBUG_STATUS,
	    printf("Sysace::thread service active for %p\n", sc));

	s = splbio();
	for (;;) {
		/* Get next I/O request, wait if necessary */
		if ((sc->ch_flags & (ATACH_TH_RESET | ATACH_SHUTDOWN)) == 0 &&
		    (sc->active_xfer == NULL)) {
			sc->ch_flags &= ~ATACH_TH_RUN;
			(void) tsleep(&sc->ch_thread, PRIBIO, "aceth", 0);
			sc->ch_flags |= ATACH_TH_RUN;
		}
		if (sc->ch_flags & ATACH_SHUTDOWN)
			break;
		bp = sc->active_xfer;
		sc->active_xfer = NULL;
		if (bp != NULL) {
			size_t sz, bnow;

			DBGME(DEBUG_XFERS,
			    printf("Sysace::task %p %p %x %p %qx %d (%zd)\n",
			    sc, bp, sc->sc_bio.flags, sc->sc_bio.databuf,
			    sc->sc_bio.blkno, sc->sc_bio.nbytes,
			    sc->sc_bio.nblks));

			sc->sc_bio.error = 0;
			for (; sc->sc_bio.nblks > 0;) {

				bnow = sc->sc_bio.nblks;
				if (sc->sc_bio.flags & ATA_SINGLE)
					bnow = 1;

				if (sc->sc_bio.flags & ATA_READ) {
					sc->sc_bio.error =
					    sysace_read_at(sc,
					    sc->sc_bio.blkno,
					    sc->sc_bio.databuf, bnow, &sz);
				} else {
					sc->sc_bio.error =
					    sysace_write_at(sc,
					    sc->sc_bio.blkno,
					    sc->sc_bio.databuf, bnow, &sz);
				}

				if (FAILED(sc->sc_bio.error))
					break;

				sc->sc_bio.blkno += sz; /* in blocks */
				sc->sc_bio.nblks -= sz;
				sc->sc_bio.blkdone += sz;
				sz = sz << CF_SECBITS; /* in bytes */
				sc->sc_bio.databuf += sz;
				sc->sc_bio.nbytes  -= sz;
			}

			acedone(sc);
		}
	}

	splx(s);
	sc->ch_thread = NULL;
	wakeup(&sc->ch_flags);
	kthread_exit(0);
}

/* Worker routines
 */
#if _DEBUG
typedef char *NAME;
typedef struct _REGDESC {
	NAME RegisterName;
	NAME BitNames[32];
} REGDESC, *PREGDESC;

static void
SysacePrintRegister(const REGDESC *Desc, uint32_t Value)
{
	int i;

	printf("\t%s %x =", Desc->RegisterName, Value);
	for (i = 31; i >= 0; i--) {
		if (Value & (1 << i))
			printf(" %s",
			    (Desc->BitNames[i]) ? Desc->BitNames[i] : "?");
	}
	printf("\n");
}

static uint32_t
SysaceDumpRegisters(struct _Sac *regs)
{
	const REGDESC Control_Names = {
		"Control",
		{
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			"RST",		/* 0x00010000 */
			"BUS8",		/* 0x00020000 */
			"BUS16",	/* 0x00040000 */
			"BUS32",	/* 0x00080000 */
			"IRQ",		/* 0x00100000 */
			"BRDY",		/* 0x00200000 */
			"IMSK0",	/* 0x00400000 */
			"IMSK1",	/* 0x00800000 */
			"TD0",		/* 0x0f000000 */
			"TD1",		/* 0x0f000000 */
			"TD2",		/* 0x0f000000 */
			"TD3",		/* 0x0f000000 */
			"BUFW8",	/* 0x10000000 */
			"BUFW16",	/* 0x20000000 */
			"BUFW32",	/* 0x40000000 */
			"DEBUG"		/* 0x80000000 */
		}
	};

	const REGDESC STATUS_Names = {
		"STATUS",
		{
			"CFGLOCK",	/* 0x00000001 */
			"MPULOCK",	/* 0x00000002 */
			"CFGERROR",	/* 0x00000004 */
			"CFCERROR",	/* 0x00000008 */
			"CFDETECT",	/* 0x00000010 */
			"DATABUFRDY",	/* 0x00000020 */
			"DATABUFWRITE",	/* 0x00000040 */
			"CFGDONE",	/* 0x00000080 */
			"RDYFORCFCMD",	/* 0x00000100 */
			"CFGMODEPIN",	/* 0x00000200 */
			0,0,0,
			"CFGADDRPIN0",	/* 0x0000e000 */
			"CFGADDRPIN1",	/* 0x0000e000 */
			"CFGADDRPIN2",	/* 0x0000e000 */
			0,
			"CFBSY",	/* 0x00020000 */
			"CFRDY",	/* 0x00040000 */
			"CFDWF",	/* 0x00080000 */
			"CFDSC",	/* 0x00100000 */
			"CFDRQ",	/* 0x00200000 */
			"CFCORR",	/* 0x00400000 */
			"CFERR",	/* 0x00800000 */
			0,
		}
	};

	const REGDESC ERRORREG_Names = {
		"ERRORREG",
		{
			"CARDRESETERR",	/* 0x00000001 */
			"CARDRDYERR",	/* 0x00000002 */
			"CARDREADERR",	/* 0x00000004 */
			"CARDWRITEERR",	/* 0x00000008 */
			"SECTORRDYERR",	/* 0x00000010 */
			"CFGADDRERR",	/* 0x00000020 */
			"CFGFAILED",	/* 0x00000040 */
			"CFGREADERR",	/* 0x00000080 */
			"CFGINSTRERR",	/* 0x00000100 */
			"CFGINITERR",	/* 0x00000200 */
			0,
			"CFBBK",	/* 0x00000800 */
			"CFUNC",	/* 0x00001000 */
			"CFIDNF",	/* 0x00002000 */
			"CFABORT",	/* 0x00004000 */
			"CFAMNF",	/* 0x00008000 */
			0,
		}
	};

	const NAME CommandNames[8] = {
		"0",			/* 0x0000 */
		"RESETMEMCARD",		/* 0x0100 */
		"IDENTIFYMEMCARD",	/* 0x0200 */
		"READMEMCARDDATA",	/* 0x0300 */
		"WRITEMEMCARDDATA",	/* 0x0400 */
		"5",			/* 0x0500 */
		"ABORT",		/* 0x0600 */
		"7"			/* 0x0700 */
	};

	const REGDESC CONTROLREG_Names = {
		"CONTROLREG",
		{
			"FORCELOCKREQ",	/* 0x00000001 */
			"LOCKREQ",	/* 0x00000002 */
			"FORCECFGADDR",	/* 0x00000004 */
			"FORCECFGMODE",	/* 0x00000008 */
			"CFGMODE",	/* 0x00000010 */
			"CFGSTART",	/* 0x00000020 */
			"CFGSEL_MPU",	/* 0x00000040 */
			"CFGRESET",	/* 0x00000080 */
			"DATABUFRDYIRQ",/* 0x00000100 */
			"ERRORIRQ",	/* 0x00000200 */
			"CFGDONEIRQ",	/* 0x00000400 */
			"RESETIRQ",	/* 0x00000800 */
			"CFGPROG",	/* 0x00001000 */
			"CFGADDR_B0",	/* 0x00002000 */
			"CFGADDR_B1",	/* 0x00004000 */
			"CFGADDR_B2",	/* 0x00008000 */
			0,
		}
	};

	const REGDESC FATSTATREG_Names = {
		"FATSTATREG",
		{
			"MBRVALID",	/* 0x00000001 */
			"PBRVALID",	/* 0x00000002 */
			"MBRFAT12",	/* 0x00000004 */
			"PBRFAT12",	/* 0x00000008 */
			"MBRFAT16",	/* 0x00000010 */
			"PBRFAT16",	/* 0x00000020 */
			"CALCFAT12",	/* 0x00000040 */
			"CALCFAT16",	/* 0x00000080 */
			0,
		}
	};

	printf("Sysace@%p:\n", regs);
	printf("\tTag %x\n", regs->Tag);
	SysacePrintRegister(&Control_Names, regs->Control);
	printf("\tBUSMODEREG %x\n", regs->BUSMODEREG);
	SysacePrintRegister(&STATUS_Names, regs->STATUS);
	SysacePrintRegister(&ERRORREG_Names, regs->ERRORREG);
	printf("\tCFGLBAREG %x\n", regs->CFGLBAREG);
	printf("\tMPULBAREG %x\n", regs->MPULBAREG);
	printf("\tVERSIONREG %x\n", regs->VERSIONREG);
	printf("\tSECCNTCMDREG %x = %s cnt=%d\n", regs->SECCNTCMDREG,
	    CommandNames[(regs->SECCNTCMDREG >> 8) & 7],
	    regs->SECCNTCMDREG & SAC_SECCCNT);
	SysacePrintRegister(&CONTROLREG_Names, regs->CONTROLREG);
	SysacePrintRegister(&FATSTATREG_Names, regs->FATSTATREG);

	return 1;
}

#else
#define SysaceDumpRegisters(_c_)
#endif

/*
 * Reset the device and the interface
 */
static int
sysace_reset(struct ace_softc *sc)
{
	struct _Sac *regs = sc->sc_dr;

	DBGME(DEBUG_FUNCS, printf("Sysace::Reset %p\n", sc));

	/* 16bit etc etc */
	uint32_t BusMode, Control;

	/* reset our interface */
	regs->Control = SAC_RST;
	DELAY(200);

	/* repeat on both byte lanes */
	regs->BUSMODEREG = SAC_MODE16 | (SAC_MODE16 << 8);
	DELAY(1);

	/* check what our interface does and what the SysACE expects */
	Control = regs->Control;
	BusMode = regs->BUSMODEREG;

	/* get them to agree */
	if (BusMode & SAC_MODE16) {
		regs->Control = Control | SAC_BUS16;
		regs->Control = regs->Control & ~SAC_BUS8;
	} else {
		regs->Control = Control | SAC_BUS8;
		regs->Control = regs->Control & ~SAC_BUS16;
	}

	/* check that it worked */
	BusMode = regs->BUSMODEREG;
	Control = regs->Control;

	if (((BusMode & SAC_MODE16) == 0) && ((Control & SAC_BUS8) == 0))
		return EDOOFUS;
	if (((BusMode & SAC_MODE16) > 0) && ((Control & SAC_BUS16) == 0))
		return EDOOFUS;

	/* interrupts off for now */
	regs->Control &= ~SAC_INTMASK;
#define SAC_INTERRUPTS (SAC_DATABUFRDYIRQ | SAC_ERRORIRQ /* | SAC_CFGDONEIRQ */)
	Control = regs->CONTROLREG;
	Control = (Control & ~SAC_INTERRUPTS) | SAC_RESETIRQ | SAC_FORCECFGMODE;
	regs->CONTROLREG = Control;
	regs->CONTROLREG = Control & ~SAC_RESETIRQ;

	/* no command */
	regs->MPULBAREG = 0;

	return 0;
}

/*
 * Take control of the ACE datapath
 */
static int
sysace_lock_registers(struct ace_softc *sc)
{
	uint32_t Status;
	int i;

	DBGME(DEBUG_FUNCS, printf("Sysace::Lock %p\n", sc));

	/*
	 * Locked already?
	 */
	Status = sc->sc_dr->STATUS;
	if (Status & SAC_MPULOCK)
		return TRUE;

	/*
	 * Request lock
	 */
	sc->sc_dr->CONTROLREG |= SAC_LOCKREQ;

	/*
	 * Spin a bit until we get it
	 */
	for (i = 0; i < 200; i++) {
		Status = sc->sc_dr->STATUS;
		if (Status & SAC_MPULOCK)
			return TRUE;
		DELAY(100);
		DBGME(DEBUG_FUNCS,
		    printf("Sysace::Lock loops.. (st=%x)\n",Status));
	}

	/*
	 * oopsie!
	 */
	DBGME(DEBUG_ERRORS, printf("Sysace::Lock timeout (st=%x)\n",Status));
	SysaceDumpRegisters(sc->sc_dr);
	return FALSE;
}

/*
 * Release control of the ACE datapath
 */
static int
sysace_unlock_registers(struct ace_softc *sc)
{
	uint32_t Status;
	int i;

	DBGME(DEBUG_FUNCS, printf("Sysace::Unlock %p\n", sc));

	/*
	 * Clear reset
	 */
	sc->sc_dr->CONTROLREG &= ~SAC_CFGRESET;

	/*
	 * Unlocked already?
	 */
	Status = sc->sc_dr->STATUS;
	if ((Status & SAC_MPULOCK) == 0)
		return TRUE;

	/*
	 * Request unlock
	 */
	sc->sc_dr->CONTROLREG &= ~SAC_LOCKREQ;

	/*
	 * Spin a bit until we get it
	 */
	for (i = 0; i < 200; i++) {
		Status = sc->sc_dr->STATUS;
		if ((Status & SAC_MPULOCK) == 0)
			return TRUE;
		DELAY(100);
		DBGME(DEBUG_FUNCS,
		    printf("Sysace::Unlock loops.. (st=%x)\n",Status));
	}

	/*
	 * oopsie!
	 */
	DBGME(DEBUG_ERRORS, printf("Sysace::Unlock timeout (st=%x)\n",Status));
	SysaceDumpRegisters(sc->sc_dr);
	return FALSE;
}

/*
 * Check if the ACE is waiting for a comamnd
 */
#define sysace_ready(_s_) ((_s_)->sc_dr->STATUS & SAC_RDYFORCFCMD)

/*
 * Check if the ACE is executing a comamnd
 */
#define sysace_busy(_s_) ((_s_)->sc_dr->STATUS & SAC_CFBSY)

/*
 * Turn on interrupts from the ACE
 */
#define sysace_inton(_s_) { \
	(_s_)->sc_dr->CONTROLREG |= SAC_INTERRUPTS; \
	(_s_)->sc_dr->Control |= SAC_INTMASK; \
}

/*
 * Turn off interrupts from the ACE
 */
#define sysace_intoff(_s_) { \
	(_s_)->sc_dr->CONTROLREG &= ~SAC_INTERRUPTS; \
	(_s_)->sc_dr->Control &= ~SAC_INTMASK; \
}

/*
 * Start a command on the ACE, such as read or identify.
 */
static int
sysace_start(struct ace_softc *sc, uint32_t Command, uint32_t Lba,
    uint32_t nSectors)
{

	/*
	 * Lock it if not already
	 */
	if (!sysace_lock_registers(sc)) {
		/* printed already */
		return ETIMEDOUT;
	}

	/*
	 * Is there a CF inserted
	 */
	if (!(sc->sc_dr->STATUS & SAC_CFDETECT)) {
		/* NB: Not a failure state */
		DBGME(DEBUG_ERRORS,
		    printf("Sysace:: no media (st=%x)\n", sc->sc_dr->STATUS));
		if (sc->sc_capacity) {
			sc->media_has_changed = TRUE;
			sc->sc_capacity = 0;
		}
		return ENODEV;
	}

	/*
	 * Is it ready for a command
	 */
	if (!sysace_ready(sc)) {
		DBGME(DEBUG_ERRORS,
		    printf("Sysace:: not ready (st=%x)\n", sc->sc_dr->STATUS));
		SysaceDumpRegisters(sc->sc_dr);
		return EBUSY;
	}

	/*
	 * sector number and command
	 */
	sc->sc_dr->MPULBAREG = Lba;
	sc->sc_dr->SECCNTCMDREG =
	    (uint16_t)(Command | (nSectors & SAC_SECCCNT));

	/*
	 *  re-route the chip
	 * NB: The "RESET" is actually not much of a misnomer.
	 * The chip was designed for a one-shot execution at reset time,
	 * namely loading the configuration data into the FPGA. So.
	 */
	sc->hw_busy = TRUE;
	sc->sc_dr->CONTROLREG |= SAC_CFGRESET;
	return 0;
}

/*
 * Identify the (size of the) CompactFlash card inserted in the slot.
 */
static int
sysace_identify(struct ace_softc *sc)
{
	PCFLASH_IDENTIFY Identify = &sc->sc_params;
	uint32_t Status = 0;
	int i, j, error;

	DBGME(DEBUG_FUNCS, printf("Sysace::Identify %p\n", sc));

	/*
	 * Turn on interrupts before we start the command
	 */
	sysace_inton(sc); /* BUGBUG we should add polling mode (for dump too) */

	/*
	 * This will invalidate the ACE's current sector data
	 */
	sc->sc_capacity = 0;

	/*
	 * Get it going
	 */
	error = sysace_start(sc, SAC_CMD_IDENTIFYMEMCARD, 0, 1);

	/*
	 * Wait until its done
	 */
	if (!FAILED(error)) {

		/* Might be called during autoconf, no interrupts */
		if (cold) {
			do {
				DELAY(10);
				Status = sc->sc_dr->STATUS;
			} while ((Status &
			    (SAC_DATABUFRDY|SAC_CFCERROR|SAC_CFGERROR)) == 0);
		} else {
			while (sc->hw_busy) {
				DBGME(DEBUG_FUNCS,
				    printf("Sysace:: cwait.. (st=%x)"
				    " sizeof=%d\n",
				    sc->sc_dr->STATUS, sizeof(*Identify)));
				error = tsleep(&sc->media_has_changed, PRIBIO,
				    "aceidfy", 0);
			}
		}

		/*
		 * Did it work?
		 */
		Status = sc->sc_dr->STATUS;

		if (Status & SAC_DATABUFRDY) {

			/*
			 * Yes, pull out all the data.
			 * NB: Until we do so the chip will not be ready for
			 *     another command
			 */
			for (i = 0; i < sizeof(*Identify); i += 4) {

				/*
				 * Verify the (32-bytes) FIFO has reloaded
				 */
				for (j = 0; j < 10; j++) {
					Status = sc->sc_dr->STATUS;
					if (Status & SAC_DATABUFRDY)
						break;
					DELAY(10);
				}
				if (Status & SAC_DATABUFRDY) {
					uint32_t Data32;

					/*
					 * This pulls two 16-bit words out of
					 * the FIFO.
					 * They are ordered in LE.
					 * NB: Yes this is different from
					 *     regular data accesses
					 */
					Data32 = sc->sc_dr->DATABUFREG[0];
#if _BYTE_ORDER == _LITTLE_ENDIAN
					/* all is fine */
#else
					Data32 =
					    (Data32 >> 16) | (Data32 << 16);
#endif
					memcpy(((char *)Identify) + i,
					    &Data32, 4);
				} else {
					/*
					 * Ooops, what's going on here?
					 */
					DBGME(DEBUG_ERRORS,
					    printf("Sysace::!DATABUFRDY %x\n",
					    Status));
					error = EIO;
					break;
				}
			}

			/*
			 * Make sure we did ok and pick up the relevant info
			 */
			if (Status & SAC_DATABUFRDY) {
				DBGME(DEBUG_XFERS,
				    device_printf(sc->sc_dev,
				    "model: %.40s/%.20s\n",
				    Identify->ModelNumber,
				    Identify->SerialNumber));
				if (Identify->Signature == CFLASH_SIGNATURE) {
					DBGME(DEBUG_PROBE,
					    printf("Sysace::Card is"
					    " %.40s::%.20s\n",
					    Identify->ModelNumber,
					    Identify->SerialNumber));

					sc->sc_capacity =
					    (Identify->SectorsPerCard[0] << 16)
					    | Identify->SectorsPerCard[1];
					DBGME(DEBUG_PROBE,
					    printf("Sysace::sc_capacity x%qx\n",
					    sc->sc_capacity));
					ace_set_geometry(sc);
				} else {
					DBGME(DEBUG_ERRORS,
					    printf("Sysace::Bad card signature?"
					    " %x != %x\n",
					    Identify->Signature,
					    CFLASH_SIGNATURE));
					sc->sc_capacity = 0;
					error = ENXIO;
				}
			} else {
				error = ETIMEDOUT;
			}
		} else {
			/*
			 * No, it did not work. Maybe there is no card inserted
			 */
			DBGME(DEBUG_ERRORS,
			    printf("Sysace::Identify failed,"
			    " missing CFLASH card?\n"));
			SysaceDumpRegisters(sc->sc_dr);
			/* BUGBUG Fix the error code accordingly */
			error = ETIMEDOUT;
		}
	}

	/* remember this jic */
	sc->sc_bio.r_error = Status;

	/* Free the ACE for the JTAG, just in case */
	sysace_unlock_registers(sc);

	/*
	 * Done
	 */
	return error;
}

/*
 * Common code for read&write argument validation
 */
static int
sysace_validate(struct ace_softc *sc, daddr_t start, size_t *pSize)
{
	daddr_t Size;

	/*
	 * Verify that we know the media size
	 */
	if (sc->sc_capacity == 0) {
		int error = sysace_identify(sc);
		if (FAILED(error))
			return error;
	}

	/*
	 * Validate args
	 */
	if (start >= sc->sc_capacity) {
		*pSize = 0;
		DBGME(DEBUG_ERRORS,
		    printf("Sysace::ValidateArg(%qx) EOF\n", start));
		return E2BIG;
	}

	/*
	 * Adjust size if necessary
	 */
	Size = start + *pSize;
	if (Size > sc->sc_capacity) {
		/*
		 * At most this many sectors
		 */
		Size = sc->sc_capacity - start;
		*pSize = (size_t)Size;
	}

	DBGME(DEBUG_FUNCS,
	    printf("Sysace::Validate %qx %zd\n", start, *pSize));
	return 0;
}

/* Read SIZE bytes from sysace device, at offset Position
 */
uint32_t ace_maxatatime = 255;
#define MAXATATIME ace_maxatatime //255 /* BUGBUG test me on real hardware!! */

static int
sysace_read_at(struct ace_softc *sc, daddr_t start_sector, char *buffer,
    size_t nblocks, size_t *pSizeRead)
{
	int error;
	uint32_t BlocksThisTime;
	uint32_t Status = 0, SizeRead = 0;
	uint32_t i, j;

	DBGME(DEBUG_XFERS|DEBUG_READS,
	    printf("SysaceReadAt(%p %qx %p %zd %p)\n",
	    sc, start_sector, buffer, nblocks, pSizeRead));

	/*
	 * Validate & trim arguments
	 */
	error = sysace_validate(sc, start_sector, &nblocks);

	/*
	 * Repeat until we are done or error
	 */
	while (error == 0) {

		/*
		 * .. one bunch of sectors at a time
		 */
		BlocksThisTime = nblocks;
		if (BlocksThisTime > MAXATATIME)
			BlocksThisTime = MAXATATIME;

		/*
		 * Yes, start a sector read
		 */
		sysace_inton(sc);
		error = sysace_start(sc,
		    SAC_CMD_READMEMCARDDATA,
		    (uint32_t)start_sector,  /* BUGBUG trims here, no warn. */
		    BlocksThisTime);
		/*
		 * And wait until done, if ok
		 */
		if (!FAILED(error)) {
			start_sector += BlocksThisTime;
			/* Might be called during autoconf, no interrupts */
			/* BUGBUG timeouts! */
			if (cold) {
				do {
					DELAY(10);
					Status = sc->sc_dr->STATUS;
				} while ((Status &
				    (SAC_DATABUFRDY|SAC_CFCERROR|SAC_CFGERROR))
				    == 0);
			} else {
				while (sc->hw_busy) {
					error = tsleep(&sc->media_has_changed,
					    PRIBIO, "aceread", 0);
				}
			}
		}

		/*
		 * Are we doing ok
		 */
		if (!FAILED(error)) {

		/*
		 * Get the data out of the ACE
		 */
			for (i = 0; i < (BlocksThisTime << CF_SECBITS);
			    i += 4) {

				/*
				 * Make sure the FIFO is ready
				 */
				for (j = 0; j < 10; j++) {
					Status = sc->sc_dr->STATUS;
					if (Status & SAC_DATABUFRDY)
						break;
					DELAY(1000);
				}

				/*
				 * Got it?
				 */
				if (Status & SAC_DATABUFRDY) {
					uint32_t Data32;

					Data32 = sc->sc_dr->DATABUFREG[0];
					Data32 = le32toh(Data32);
					memcpy(buffer + i, &Data32, 4);
				} else {
					/*
					 * Ooops, get out of here
					 */
					DBGME(DEBUG_ERRORS,
					    printf("Sysace::READ timeout\n"));
					SysaceDumpRegisters(sc->sc_dr);
					error = ETIMEDOUT;
					break;
				}
			}

			/*
			 * Still doing ok?
			 */
			if (!FAILED(error)) {
				nblocks   -= BlocksThisTime;
				SizeRead  += BlocksThisTime;
				buffer    += BlocksThisTime << CF_SECBITS;
			} else {
				/* remember this jic */
				sc->sc_bio.r_error = Status;
			}
		}

		/* Free the ACE for the JTAG, just in case */
		sysace_unlock_registers(sc);

		/*
		 * Are we done yet?
		 */
		if (nblocks == 0)
			break;
	}

	if (pSizeRead)
		*pSizeRead = SizeRead;
	return error;
}

/*
 * Write SIZE bytes to device.
 */
static int
sysace_write_at(struct ace_softc *sc, daddr_t start_sector, char *buffer,
    size_t nblocks, size_t *pSizeWritten)
{
	int error;
	uint32_t BlocksThisTime;
	uint32_t Status = 0, SizeWritten = 0;
	uint32_t i, j;

	DBGME(DEBUG_XFERS|DEBUG_WRITES,
	    printf("SysaceWriteAt(%p %qx %p %zd %p)\n",
	    sc, start_sector, buffer, nblocks, pSizeWritten));

	/*
	 * Validate & trim arguments
	 */
	error = sysace_validate(sc, start_sector, &nblocks);

	/*
	 * Repeat until we are done or error
	 */
	while (error == 0) {

		/*
		 * .. one sector at a time
		 * BUGBUG Supposedly we can do up to 256 sectors?
		 */
		BlocksThisTime = nblocks;
		if (BlocksThisTime > MAXATATIME)
			BlocksThisTime = MAXATATIME;

		/*
		 * Yes, start a sector write
		 */
		sysace_inton(sc);
		error = sysace_start(sc,
		    SAC_CMD_WRITEMEMCARDDATA,
		    (uint32_t)start_sector,  /* BUGBUG trims here, no warn. */
		    BlocksThisTime);
		/*
		 * And wait until done, if ok
		 */
		if (!FAILED(error)) {
			start_sector += BlocksThisTime;
			/* BUGBUG timeouts! */
			while (sc->hw_busy) {
				error = tsleep(&sc->media_has_changed,
				    PRIBIO, "acewrite", 0);
			}
		}

		/*
		 * Are we doing ok
		 */
		if (!FAILED(error)) {

			/*
			 * Get the data out to the ACE
			 */
			for (i = 0; i < (BlocksThisTime << CF_SECBITS);
			    i += 4) {

				/*
				 * Make sure the FIFO is ready
				 */
				for (j = 0; j < 10; j++) {
					Status = sc->sc_dr->STATUS;
					if (Status & SAC_DATABUFRDY)
						break;
					DELAY(1000);
				}

				/*
				 * Got it?
				 */
				if (Status & SAC_DATABUFRDY) {
					uint32_t Data32;

					memcpy(&Data32, buffer + i, 4);
					Data32 = htole32(Data32);
					sc->sc_dr->DATABUFREG[0] = Data32;
				} else {
					/*
					 * Ooops, get out of here
					 */
					DBGME(DEBUG_ERRORS,
					    printf("Sysace::WRITE timeout\n"));
					SysaceDumpRegisters(sc->sc_dr);
					error = ETIMEDOUT;
					/* remember this jic */
					sc->sc_bio.r_error = Status;
					break;
				}
			}

			/*
			 * Still doing ok?
			 */
			if (!FAILED(error)) {
				nblocks     -= BlocksThisTime;
				SizeWritten += BlocksThisTime;
				buffer      += BlocksThisTime << CF_SECBITS;
			}
		}

		/*
		 * We need to wait until the device is ready for the
		 * next command
		 * Experimentation shows that it can take longer than 10msec.
		 */
		if (!FAILED(error)) {
			for (j = 0; j < 300; j++) {
				Status = sc->sc_dr->STATUS;
				if (Status & SAC_RDYFORCFCMD)
					break;
				(void)tsleep(&sc->media_has_changed,
				    PRIBIO, "acewrite", 2);
			}
			if (!(Status & SAC_RDYFORCFCMD)) {
				DBGME(DEBUG_ERRORS,
				    printf("Sysace::WRITE-COMPLETE timeout"
				    " St=%x\n", Status));
				SysaceDumpRegisters(sc->sc_dr);
				/*
				 * Ignore, we'll handle it the next time around
				 * BUGBUG To be revised along with non-existant
				 * error handling
				 */
			}
		}

		/* Free the ACE for the JTAG, just in case */
		sysace_unlock_registers(sc);

		/*
		 * Are we done yet?
		 */
		if (nblocks == 0)
			break;
	}

	if (pSizeWritten)
		*pSizeWritten = SizeWritten;
	return error;
}

int
ace_ebus_intr(void *cookie, void *f)
{
	struct ace_softc *sc = cookie;
	uint32_t Control;

	/*
	 * Turn off interrupts and ACK them
	 */
	sysace_intoff(sc);

	Control = sc->sc_dr->CONTROLREG & (~(SAC_RESETIRQ|SAC_INTERRUPTS));
	sc->sc_dr->CONTROLREG = Control | SAC_RESETIRQ;
	sc->sc_dr->CONTROLREG = Control;

	/* ... read status and do whatever ... */

	sc->hw_busy = FALSE;
	wakeup(&sc->media_has_changed);
	return 1;
}

#ifdef USE_ACE_FOR_RECONFIG
static int
sysace_send_config(struct ace_softc *sc, uint32_t *Data, unsigned int nBytes)
{
	struct _Sac *Interface = sc->sc_dr;
	unsigned int i, j, nWords;
	uint32_t CtlWas;
	uint32_t Status;

	CtlWas = Interface->CONTROLREG;

	/* Set the bits but in RESET (pag 49-50 of specs)*/
#define CFGCMD (SAC_FORCELOCKREQ | SAC_LOCKREQ | SAC_CFGSEL | \
		SAC_FORCECFGMODE |/* SAC_CFGMODE |*/ SAC_CFGSTART)

	Interface->CONTROLREG = CFGCMD | SAC_CFGRESET;

	/* Take it out of RESET */
	Interface->CONTROLREG = CFGCMD;

	/*
	 * Must wait till it says READY
	 * It can take a looong time
	 */
	for (j = 0; j < 1000; j++) {
		Status = Interface->STATUS;
		if (Status & SAC_RDYFORCFCMD)
			break;
		DELAY(1000);
	}

	if (0 == (Status & SAC_RDYFORCFCMD)) {
		DBGME(DEBUG_ERRORS,
		    printf("Sysace::CMD error %x (j=%d)\n", Status, j));
		goto Error;
	}

	/*
	 * Get the data out to the ACE
	 */
#define ACEROUNDUP 32
	nBytes = (nBytes + ACEROUNDUP - 1) & ~(ACEROUNDUP-1);
	nWords = (nBytes + 3) / 4;
	DBGME(DEBUG_FUNCS,
	    printf("Sending %d bytes (as %d words) to %p ",
	    nBytes, nWords, Interface));
	for (i = 0; i < nWords; i += 1/*word*/) {

		/* Stop on errors */
		Status = Interface->ERRORREG;
		if (Status) {
			/*
			 * Ooops, get out of here
			 */
			DBGME(DEBUG_ERRORS,
			    printf("Sysace::CFG error %x (i=%d)\n", Status, i));
			goto Error;
		}

		/*
		 * Make sure the FIFO is ready
		 */
		for (j = 0; j < 100; j++) {
			Status = Interface->STATUS;
			if (Status & SAC_DATABUFRDY)
				break;
			DELAY(1000);
		}

		/*
		 * Got it?
		 */
		if (Status & SAC_DATABUFRDY) {
			uint32_t Data32;

			Data32 = Data[i];
			Data32 = htole32(Data32);
			Interface->DATABUFREG[0] = Data32;
		} else {
			/*
			 * Ooops, get out of here
			 */
			DBGME(DEBUG_ERRORS,
			    printf("Sysace::WRITE timeout %x (i=%d)\n",
			    Status, i));
			goto Error;
		}
	}
	DBGME(DEBUG_FUNCS, printf("done ok.\n"));

	/* Put it back the way it was (try to.. :-( )*/
	Interface->CONTROLREG = CtlWas;
	return 0;

 Error:
	SysaceDumpRegisters(Interface);
	Interface->CONTROLREG = CtlWas;
	return EIO;
}
#endif /* USE_ACE_FOR_RECONFIG */


/*
 * Rest of code lifted with mods from the dev\ata\wd.c driver
 */

/*
 * Copyright (c) 1998, 2001 Manuel Bouyer.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *	must display the following acknowledgement:
 *  This product includes software developed by Manuel Bouyer.
 * 4. The name of the author may not be used to endorse or promote products
 *	derived from this software without specific prior written permission.
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
 */

/*-
 * Copyright (c) 1998, 2003, 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Onno van der Linden.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

static const char ST506[] = "ST506";

#define	ACEIORETRIES_SINGLE 4	/* number of retries before single-sector */
#define	ACEIORETRIES	5	/* number of retries before giving up */
#define	RECOVERYTIME hz/2	/* time to wait before retrying a cmd */

#define	ACEUNIT(dev)		DISKUNIT(dev)
#define	ACEPART(dev)		DISKPART(dev)
#define	ACEMINOR(unit, part)	DISKMINOR(unit, part)
#define	MAKEACEDEV(maj, unit, part)	MAKEDISKDEV(maj, unit, part)

#define	ACELABELDEV(dev)	(MAKEACEDEV(major(dev), ACEUNIT(dev), RAW_PART))

void	aceperror(const struct ace_softc *);

extern struct cfdriver ace_cd;

dev_type_open(aceopen);
dev_type_close(aceclose);
dev_type_read(aceread);
dev_type_write(acewrite);
dev_type_ioctl(aceioctl);
dev_type_strategy(acestrategy);
dev_type_dump(acedump);
dev_type_size(acesize);

const struct bdevsw ace_bdevsw = {
	.d_open = aceopen,
	.d_close = aceclose,
	.d_strategy = acestrategy,
	.d_ioctl = aceioctl,
	.d_dump = acedump,
	.d_psize = acesize,
	.d_discard = nodiscard,
	.d_flag = D_DISK
};

const struct cdevsw ace_cdevsw = {
	.d_open = aceopen,
	.d_close = aceclose,
	.d_read = aceread,
	.d_write = acewrite,
	.d_ioctl = aceioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_DISK
};

void  acegetdefaultlabel(struct ace_softc *, struct disklabel *);
void  acegetdisklabel(struct ace_softc *);
void  acestart(void *);
void  __acestart(struct ace_softc*, struct buf *);
void  acerestart(void *);

struct dkdriver acedkdriver = { acestrategy, minphys };

#ifdef HAS_BAD144_HANDLING
static void bad144intern(struct ace_softc *);
#endif

void
aceattach(struct ace_softc *ace)
{
	device_t self = ace->sc_dev;
	char tbuf[41], pbuf[9], c, *p, *q;
	int i, blank;
	DEBUG_PRINT(("aceattach\n"), DEBUG_FUNCS | DEBUG_PROBE);

	callout_init(&ace->sc_restart_ch, 0);
	bufq_alloc(&ace->sc_q, BUFQ_DISK_DEFAULT_STRAT, BUFQ_SORT_RAWBLOCK);

	ace->openings = 1; /* wazziz?*/
	ace->sc_multi = MAXATATIME;

	aprint_naive("\n");

	/* setup all required fields so that if the attach fails we are ok */
	ace->sc_dk.dk_driver = &acedkdriver;
	ace->sc_dk.dk_name = device_xname(ace->sc_dev);

	/* read our drive info */
	if (sysace_attach(ace) != 0) {
		aprint_error_dev(ace->sc_dev, "attach failed\n");
		return;
	}

	aprint_normal_dev(ace->sc_dev, "drive supports %d-sector PIO xfers\n",
	    ace->sc_multi);

	for (blank = 0, p = ace->sc_params.ModelNumber, q = tbuf, i = 0;
	    i < sizeof(ace->sc_params.ModelNumber); i++) {
		c = *p++;
		if (c == '\0')
			break;
		if (c != ' ') {
			if (blank) {
				*q++ = ' ';
				blank = 0;
			}
			*q++ = c;
		} else
			blank = 1;
	}
	*q++ = '\0';

	aprint_normal_dev(ace->sc_dev, "card is <%s>\n", tbuf);

	format_bytes(pbuf, sizeof(pbuf), ace->sc_capacity * DEV_BSIZE);
	aprint_normal("%s: %s, %d cyl, %d head, %d sec, "
	    "%d bytes/sect x %llu sectors\n",
	    device_xname(self), pbuf,
	    (int)(ace->sc_capacity /
	    (ace->sc_params.CurrentNumberOfHeads *
	    ace->sc_params.CurrentSectorsPerTrack)),
	    ace->sc_params.CurrentNumberOfHeads,
	    ace->sc_params.CurrentSectorsPerTrack,
	    DEV_BSIZE, (unsigned long long)ace->sc_capacity);

	/*
	 * Attach the disk structure. We fill in dk_info later.
	 */
	disk_attach(&ace->sc_dk);

	rnd_attach_source(&ace->rnd_source, device_xname(ace->sc_dev),
			  RND_TYPE_DISK, RND_FLAG_DEFAULT);

}

int
aceactivate(device_t self, enum devact act)
{
	int rv = 0;

	switch (act) {
	case DVACT_DEACTIVATE:
		/*
		 * Nothing to do; we key off the device's DVF_ACTIVATE.
		 */
		break;
	default:
		rv = EOPNOTSUPP;
	}
	return rv;
}

int
acedetach(device_t self, int flags)
{
	struct ace_softc *sc = device_private(self);
	int s, bmaj, cmaj, i, mn;

	/* locate the major number */
	bmaj = bdevsw_lookup_major(&ace_bdevsw);
	cmaj = cdevsw_lookup_major(&ace_cdevsw);

	/* Nuke the vnodes for any open instances. */
	for (i = 0; i < MAXPARTITIONS; i++) {
		mn = ACEMINOR(device_unit(self), i);
		vdevgone(bmaj, mn, mn, VBLK);
		vdevgone(cmaj, mn, mn, VCHR);
	}

	/* Delete all of our wedges. */
	dkwedge_delall(&sc->sc_dk);

	s = splbio();

	/* Kill off any queued buffers. */
	bufq_drain(sc->sc_q);

	bufq_free(sc->sc_q);
#if 0
	sc->atabus->ata_killpending(sc->drvp);
#endif

	splx(s);

	/* Detach disk. */
	disk_detach(&sc->sc_dk);

	/* Unhook the entropy source. */
	rnd_detach_source(&sc->rnd_source);

#if 0
	sc->drvp->drive_flags = 0; /* no drive any more here */
#endif

	return 0;
}

/*
 * Read/write routine for a buffer.  Validates the arguments and schedules the
 * transfer.  Does not wait for the transfer to complete.
 */
void
acestrategy(struct buf *bp)
{
	struct ace_softc *ace;
	struct disklabel *lp;
	daddr_t blkno;
	int s;

	ace = device_lookup_private(&ace_cd, ACEUNIT(bp->b_dev));

	if (ace == NULL) {
		bp->b_error = ENXIO;
		biodone(bp);
		return;
	}
	lp = ace->sc_dk.dk_label;

	DEBUG_PRINT(("acestrategy (%s) %lld\n",
	    device_xname(ace->sc_dev), bp->b_blkno), DEBUG_XFERS);

	/* Valid request?  */
	if (bp->b_blkno < 0 ||
	    (bp->b_bcount % lp->d_secsize) != 0 ||
	    (bp->b_bcount / lp->d_secsize) >= (1 << NBBY)) {
		bp->b_error = EINVAL;
		goto done;
	}

	/* If device invalidated (e.g. media change, door open), error. */
	if ((ace->sc_flags & ACEF_LOADED) == 0) {
		bp->b_error = EIO;
		goto done;
	}

	/* If it's a null transfer, return immediately. */
	if (bp->b_bcount == 0)
		goto done;

	/*
	 * Do bounds checking, adjust transfer. if error, process.
	 * If end of partition, just return.
	 */
	if (ACEPART(bp->b_dev) == RAW_PART) {
		if (bounds_check_with_mediasize(bp, DEV_BSIZE,
		    ace->sc_capacity) <= 0)
			goto done;
	} else {
		if (bounds_check_with_label(&ace->sc_dk, bp,
		    (ace->sc_flags & (ACEF_WLABEL|ACEF_LABELLING)) != 0) <= 0)
			goto done;
	}

	/*
	 * Now convert the block number to absolute and put it in
	 * terms of the device's logical block size.
	 */
	if (lp->d_secsize >= DEV_BSIZE)
		blkno = bp->b_blkno / (lp->d_secsize / DEV_BSIZE);
	else
		blkno = bp->b_blkno * (DEV_BSIZE / lp->d_secsize);

	if (ACEPART(bp->b_dev) != RAW_PART)
		blkno += lp->d_partitions[ACEPART(bp->b_dev)].p_offset;

	bp->b_rawblkno = blkno;

	/* Queue transfer on drive, activate drive and controller if idle. */
	s = splbio();
	bufq_put(ace->sc_q, bp);
	acestart(ace);
	splx(s);
	return;
done:
	/* Toss transfer; we're done early. */
	bp->b_resid = bp->b_bcount;
	biodone(bp);
}

/*
 * Queue a drive for I/O.
 */
void
acestart(void *arg)
{
	struct ace_softc *ace = arg;
	struct buf *bp = NULL;

	DEBUG_PRINT(("acestart %s\n", device_xname(ace->sc_dev)), DEBUG_XFERS);
	while (ace->openings > 0) {

		/* Is there a buf for us ? */
		if ((bp = bufq_get(ace->sc_q)) == NULL)
			return;

		/*
		 * Make the command. First lock the device
		 */
		ace->openings--;

		ace->retries = 0;
		__acestart(ace, bp);
	}
}

void
__acestart(struct ace_softc *sc, struct buf *bp)
{

	sc->sc_bp = bp;
	/*
	 * If we're retrying, retry in single-sector mode. This will give us
	 * the sector number of the problem, and will eventually allow the
	 * transfer to succeed.
	 */
	if (sc->retries >= ACEIORETRIES_SINGLE)
		sc->sc_bio.flags = ATA_SINGLE;
	else
		sc->sc_bio.flags = 0;
	if (bp->b_flags & B_READ)
		sc->sc_bio.flags |= ATA_READ;
	sc->sc_bio.blkno = bp->b_rawblkno;
	sc->sc_bio.blkdone = 0;
	sc->sc_bio.nbytes = bp->b_bcount;
	sc->sc_bio.nblks  = bp->b_bcount >> CF_SECBITS;
	sc->sc_bio.databuf = bp->b_data;
	/* Instrumentation. */
	disk_busy(&sc->sc_dk);
	sc->active_xfer = bp;
	wakeup(&sc->ch_thread);
}

void
acedone(struct ace_softc *ace)
{
	struct buf *bp = ace->sc_bp;
	const char *errmsg;
	int do_perror = 0;

	DEBUG_PRINT(("acedone %s\n", device_xname(ace->sc_dev)), DEBUG_XFERS);

	if (bp == NULL)
		return;

	bp->b_resid = ace->sc_bio.nbytes;
	switch (ace->sc_bio.error) {
	case ETIMEDOUT:
		errmsg = "device timeout";
	do_perror = 1;
		goto retry;
	case EBUSY:
	case EDOOFUS:
		errmsg = "device stuck";
retry:		/* Just reset and retry. Can we do more ? */
		sysace_reset(ace);
		diskerr(bp, "ace", errmsg, LOG_PRINTF,
		    ace->sc_bio.blkdone, ace->sc_dk.dk_label);
		if (ace->retries < ACEIORETRIES)
			printf(", retrying");
		printf("\n");
		if (do_perror)
			aceperror(ace);
		if (ace->retries < ACEIORETRIES) {
			ace->retries++;
			callout_reset(&ace->sc_restart_ch, RECOVERYTIME,
			    acerestart, ace);
			return;
		}

		bp->b_error = EIO;
		break;
	case 0:
		if ((ace->sc_bio.flags & ATA_CORR) || ace->retries > 0)
			printf("%s: soft error (corrected)\n",
			    device_xname(ace->sc_dev));
		break;
	case ENODEV:
	case E2BIG:
		bp->b_error = EIO;
		break;
	}
	disk_unbusy(&ace->sc_dk, (bp->b_bcount - bp->b_resid),
	    (bp->b_flags & B_READ));
	rnd_add_uint32(&ace->rnd_source, bp->b_blkno);
	biodone(bp);
	ace->openings++;
	acestart(ace);
}

void
acerestart(void *v)
{
	struct ace_softc *ace = v;
	struct buf *bp = ace->sc_bp;
	int s;
	DEBUG_PRINT(("acerestart %s\n",
	    device_xname(ace->sc_dev)), DEBUG_XFERS);

	s = splbio();
	__acestart(v, bp);
	splx(s);
}

int
aceread(dev_t dev, struct uio *uio, int flags)
{
	int r;

	DEBUG_PRINT(("aceread\n"), DEBUG_XFERS);
	r = physio(acestrategy, NULL, dev, B_READ, minphys, uio);
	DEBUG_PRINT(("aceread -> x%x resid=%x\n",r,uio->uio_resid),DEBUG_XFERS);

	return r;
}

int
acewrite(dev_t dev, struct uio *uio, int flags)
{

	DEBUG_PRINT(("acewrite\n"), DEBUG_XFERS);
	return physio(acestrategy, NULL, dev, B_WRITE, minphys, uio);
}

int
aceopen(dev_t dev, int flag, int fmt, struct lwp *l)
{
	struct ace_softc *ace;
	int part, error;

	DEBUG_PRINT(("aceopen\n"), DEBUG_FUNCS);
	ace = device_lookup_private(&ace_cd, ACEUNIT(dev));
	if (ace == NULL)
		return ENXIO;

	if (! device_is_active(ace->sc_dev))
		return ENODEV;

	part = ACEPART(dev);

	mutex_enter(&ace->sc_dk.dk_openlock);

	/*
	 * If there are wedges, and this is not RAW_PART, then we
	 * need to fail.
	 */
	if (ace->sc_dk.dk_nwedges != 0 && part != RAW_PART) {
		error = EBUSY;
		goto bad;
	}

	if (ace->sc_dk.dk_openmask != 0) {
		/*
		 * If any partition is open, but the disk has been invalidated,
		 * disallow further opens.
		 */
		if ((ace->sc_flags & ACEF_LOADED) == 0) {
			error = EIO;
			goto bad;
		}
	} else {
		if ((ace->sc_flags & ACEF_LOADED) == 0) {
			ace->sc_flags |= ACEF_LOADED;

			/* Load the physical device parameters. */
			if (ace->sc_capacity == 0) {
				error = sysace_identify(ace);
				if (error)
					goto bad;
			}

			/* Load the partition info if not already loaded. */
			acegetdisklabel(ace);
		}
	}

	/* Check that the partition exists. */
	if (part != RAW_PART &&
	    (part >= ace->sc_dk.dk_label->d_npartitions ||
	     ace->sc_dk.dk_label->d_partitions[part].p_fstype == FS_UNUSED)) {
		error = ENXIO;
		goto bad;
	}

	/* Insure only one open at a time. */
	switch (fmt) {
	case S_IFCHR:
		ace->sc_dk.dk_copenmask |= (1 << part);
		break;
	case S_IFBLK:
		ace->sc_dk.dk_bopenmask |= (1 << part);
		break;
	}
	ace->sc_dk.dk_openmask =
	    ace->sc_dk.dk_copenmask | ace->sc_dk.dk_bopenmask;

	mutex_exit(&ace->sc_dk.dk_openlock);
	return 0;

 bad:
	mutex_exit(&ace->sc_dk.dk_openlock);
	return error;
}

int
aceclose(dev_t dev, int flag, int fmt, struct lwp *l)
{
	struct ace_softc *ace = device_lookup_private(&ace_cd, ACEUNIT(dev));
	int part = ACEPART(dev);

	DEBUG_PRINT(("aceclose\n"), DEBUG_FUNCS);
	if (ace == NULL)
		return ENXIO;

	mutex_enter(&ace->sc_dk.dk_openlock);

	switch (fmt) {
	case S_IFCHR:
		ace->sc_dk.dk_copenmask &= ~(1 << part);
		break;
	case S_IFBLK:
		ace->sc_dk.dk_bopenmask &= ~(1 << part);
		break;
	}
	ace->sc_dk.dk_openmask =
	    ace->sc_dk.dk_copenmask | ace->sc_dk.dk_bopenmask;

	if (ace->sc_dk.dk_openmask == 0) {

		if (!(ace->sc_flags & ACEF_KLABEL))
			ace->sc_flags &= ~ACEF_LOADED;

	}

	mutex_exit(&ace->sc_dk.dk_openlock);
	return 0;
}

void
acegetdefaultlabel(struct ace_softc *ace, struct disklabel *lp)
{

	DEBUG_PRINT(("acegetdefaultlabel\n"), DEBUG_FUNCS);
	memset(lp, 0, sizeof(struct disklabel));

	lp->d_secsize = DEV_BSIZE;
	lp->d_ntracks = ace->sc_params.CurrentNumberOfHeads;
	lp->d_nsectors = ace->sc_params.CurrentSectorsPerTrack;
	lp->d_ncylinders = ace->sc_capacity /
	    (ace->sc_params.CurrentNumberOfHeads *
	     ace->sc_params.CurrentSectorsPerTrack);
	lp->d_secpercyl = lp->d_ntracks * lp->d_nsectors;

	lp->d_type = DTYPE_ST506; /* ?!? */

	strncpy(lp->d_typename, ace->sc_params.ModelNumber, 16);
	strncpy(lp->d_packname, "fictitious", 16);
	if (ace->sc_capacity > UINT32_MAX)
		lp->d_secperunit = UINT32_MAX;
	else
		lp->d_secperunit = ace->sc_capacity;
	lp->d_rpm = 3600;
	lp->d_interleave = 1;
	lp->d_flags = 0;

	lp->d_partitions[RAW_PART].p_offset = 0;
	lp->d_partitions[RAW_PART].p_size =
	    lp->d_secperunit * (lp->d_secsize / DEV_BSIZE);
	lp->d_partitions[RAW_PART].p_fstype = FS_UNUSED;
	lp->d_npartitions = RAW_PART + 1;

	lp->d_magic = DISKMAGIC;
	lp->d_magic2 = DISKMAGIC;
	lp->d_checksum = dkcksum(lp);
}

/*
 * Fabricate a default disk label, and try to read the correct one.
 */
void
acegetdisklabel(struct ace_softc *ace)
{
	struct disklabel *lp = ace->sc_dk.dk_label;
	const char *errstring;

	DEBUG_PRINT(("acegetdisklabel\n"), DEBUG_FUNCS);

	memset(ace->sc_dk.dk_cpulabel, 0, sizeof(struct cpu_disklabel));

	acegetdefaultlabel(ace, lp);

#ifdef HAS_BAD144_HANDLING
	ace->sc_bio.badsect[0] = -1;
#endif

	errstring = readdisklabel(MAKEACEDEV(0, device_unit(ace->sc_dev),
				  RAW_PART), acestrategy, lp,
				  ace->sc_dk.dk_cpulabel);
	if (errstring) {
		printf("%s: %s\n", device_xname(ace->sc_dev), errstring);
		return;
	}

#if DEBUG
	if (ACE_DEBUG(DEBUG_WRITES)) {
		int i, n = ace->sc_dk.dk_label->d_npartitions;
		printf("%s: %d parts\n", device_xname(ace->sc_dev), n);
		for (i = 0; i < n; i++) {
			printf("\t[%d]: t=%x s=%d o=%d\n", i,
			    ace->sc_dk.dk_label->d_partitions[i].p_fstype,
			    ace->sc_dk.dk_label->d_partitions[i].p_size,
			    ace->sc_dk.dk_label->d_partitions[i].p_offset);
		}
	}
#endif

#ifdef HAS_BAD144_HANDLING
	if ((lp->d_flags & D_BADSECT) != 0)
		bad144intern(ace);
#endif
}

void
aceperror(const struct ace_softc *ace)
{
	const char *devname = device_xname(ace->sc_dev);
	uint32_t Status = ace->sc_bio.r_error;

	printf("%s: (", devname);

	if (Status == 0)
		printf("error not notified");
	else
		printf("status=x%x", Status);

	printf(")\n");
}

int
aceioctl(dev_t dev, u_long xfer, void *addr, int flag, struct lwp *l)
{
	struct ace_softc *ace = device_lookup_private(&ace_cd, ACEUNIT(dev));
	int error = 0, s;

	DEBUG_PRINT(("aceioctl\n"), DEBUG_FUNCS);

	if ((ace->sc_flags & ACEF_LOADED) == 0)
		return EIO;

	error = disk_ioctl(&ace->sc_dk, xfer, addr, flag, l);
	if (error != EPASSTHROUGH)
		return error;

	switch (xfer) {
#ifdef HAS_BAD144_HANDLING
	case DIOCSBAD:
		if ((flag & FWRITE) == 0)
			return EBADF;
		ace->sc_dk.dk_cpulabel->bad = *(struct dkbad *)addr;
		ace->sc_dk.dk_label->d_flags |= D_BADSECT;
		bad144intern(ace);
		return 0;
#endif
	case DIOCGDINFO:
		*(struct disklabel *)addr = *(ace->sc_dk.dk_label);
		return 0;

	case DIOCGPART:
		((struct partinfo *)addr)->disklab = ace->sc_dk.dk_label;
		((struct partinfo *)addr)->part =
		    &ace->sc_dk.dk_label->d_partitions[ACEPART(dev)];
		return 0;

	case DIOCWDINFO:
	case DIOCSDINFO:
	{
		struct disklabel *lp;

		if ((flag & FWRITE) == 0)
			return EBADF;

		lp = (struct disklabel *)addr;

		mutex_enter(&ace->sc_dk.dk_openlock);
		ace->sc_flags |= ACEF_LABELLING;

		error = setdisklabel(ace->sc_dk.dk_label,
		    lp, /*ace->sc_dk.dk_openmask : */0,
		    ace->sc_dk.dk_cpulabel);
		if (error == 0) {
			if (xfer == DIOCWDINFO)
				error = writedisklabel(ACELABELDEV(dev),
				    acestrategy, ace->sc_dk.dk_label,
				    ace->sc_dk.dk_cpulabel);
		}

		ace->sc_flags &= ~ACEF_LABELLING;
		mutex_exit(&ace->sc_dk.dk_openlock);
		return error;
	}

	case DIOCKLABEL:
		if (*(int *)addr)
			ace->sc_flags |= ACEF_KLABEL;
		else
			ace->sc_flags &= ~ACEF_KLABEL;
		return 0;

	case DIOCWLABEL:
		if ((flag & FWRITE) == 0)
			return EBADF;
		if (*(int *)addr)
			ace->sc_flags |= ACEF_WLABEL;
		else
			ace->sc_flags &= ~ACEF_WLABEL;
		return 0;

	case DIOCGDEFLABEL:
		acegetdefaultlabel(ace, (struct disklabel *)addr);
		return 0;

	case DIOCCACHESYNC:
		return 0;

	case DIOCAWEDGE:
	    {
		struct dkwedge_info *dkw = (void *) addr;

		if ((flag & FWRITE) == 0)
			return EBADF;

		/* If the ioctl happens here, the parent is us. */
		strcpy(dkw->dkw_parent, device_xname(ace->sc_dev));
		return dkwedge_add(dkw);
	    }

	case DIOCDWEDGE:
	    {
		struct dkwedge_info *dkw = (void *) addr;

		if ((flag & FWRITE) == 0)
			return EBADF;

		/* If the ioctl happens here, the parent is us. */
		strcpy(dkw->dkw_parent, device_xname(ace->sc_dev));
		return dkwedge_del(dkw);
	    }

	case DIOCLWEDGES:
	    {
		struct dkwedge_list *dkwl = (void *) addr;

		return dkwedge_list(&ace->sc_dk, dkwl, l);
	    }

	case DIOCGSTRATEGY:
	    {
		struct disk_strategy *dks = (void *)addr;

		s = splbio();
		strlcpy(dks->dks_name, bufq_getstrategyname(ace->sc_q),
		    sizeof(dks->dks_name));
		splx(s);
		dks->dks_paramlen = 0;

		return 0;
	    }

	case DIOCSSTRATEGY:
	    {
		struct disk_strategy *dks = (void *)addr;
		struct bufq_state *new;
		struct bufq_state *old;

		if ((flag & FWRITE) == 0) {
			return EBADF;
		}
		if (dks->dks_param != NULL) {
			return EINVAL;
		}
		dks->dks_name[sizeof(dks->dks_name) - 1] = 0; /* ensure term */
		error = bufq_alloc(&new, dks->dks_name,
		    BUFQ_EXACT|BUFQ_SORT_RAWBLOCK);
		if (error) {
			return error;
		}
		s = splbio();
		old = ace->sc_q;
		bufq_move(new, old);
		ace->sc_q = new;
		splx(s);
		bufq_free(old);

		return 0;
	    }

#ifdef USE_ACE_FOR_RECONFIG
	/*
	 * Ok, how do I get this standardized
	 * [nothing to do with disks either]
	 */
#define DIOC_FPGA_RECONFIGURE _IOW('d',166, struct ioctl_pt)
	case DIOC_FPGA_RECONFIGURE:
	    {
		/*
		 * BUGBUG This is totally wrong, we need to fault in
		 * all data in advance.
		 * Otherwise we get back here with the sysace in a bad state
		 * (its NOT reentrant!)
		 */
		struct ioctl_pt *pt = (struct ioctl_pt *)addr;
		return sysace_send_config(ace,(uint32_t*)pt->data,pt->com);
	    }
#endif /* USE_ACE_FOR_RECONFIG */

	default:
		/*
		 * NB: we get a DIOCGWEDGEINFO, but nobody else handles it
		 * either
		 */
		DEBUG_PRINT(("aceioctl: unsup x%lx\n", xfer), DEBUG_FUNCS);
		return ENOTTY;
	}
}

int
acesize(dev_t dev)
{
	struct ace_softc *ace;
	int part, omask;
	int size;

	DEBUG_PRINT(("acesize\n"), DEBUG_FUNCS);

	ace = device_lookup_private(&ace_cd, ACEUNIT(dev));
	if (ace == NULL)
		return -1;

	part = ACEPART(dev);
	omask = ace->sc_dk.dk_openmask & (1 << part);

	if (omask == 0 && aceopen(dev, 0, S_IFBLK, NULL) != 0)
		return -1;
	if (ace->sc_dk.dk_label->d_partitions[part].p_fstype != FS_SWAP)
		size = -1;
	else
		size = ace->sc_dk.dk_label->d_partitions[part].p_size *
		    (ace->sc_dk.dk_label->d_secsize / DEV_BSIZE);
	if (omask == 0 && aceclose(dev, 0, S_IFBLK, NULL) != 0)
		return -1;
	return size;
}

/* #define ACE_DUMP_NOT_TRUSTED if you just want to watch */
#define ACE_DUMP_NOT_TRUSTED
static int acedoingadump = 0;

/*
 * Dump core after a system crash.
 */
int
acedump(dev_t dev, daddr_t blkno, void *va, size_t size)
{
	struct ace_softc *ace;	/* disk unit to do the I/O */
	struct disklabel *lp;   /* disk's disklabel */
	int part, err;
	int nblks;	/* total number of sectors left to write */

	/* Check if recursive dump; if so, punt. */
	if (acedoingadump)
		return EFAULT;
	acedoingadump = 1;

	ace = device_lookup_private(&ace_cd, ACEUNIT(dev));
	if (ace == NULL)
		return ENXIO;

	part = ACEPART(dev);

	/* Convert to disk sectors.  Request must be a multiple of size. */
	lp = ace->sc_dk.dk_label;
	if ((size % lp->d_secsize) != 0)
		return EFAULT;
	nblks = size / lp->d_secsize;
	blkno = blkno / (lp->d_secsize / DEV_BSIZE);

	/* Check transfer bounds against partition size. */
	if ((blkno < 0) || ((blkno + nblks) > lp->d_partitions[part].p_size))
		return EINVAL;

	/* Offset block number to start of partition. */
	blkno += lp->d_partitions[part].p_offset;

	ace->sc_bp = NULL;
	ace->sc_bio.blkno = blkno;
	ace->sc_bio.flags = ATA_POLL;
	ace->sc_bio.nbytes = nblks * lp->d_secsize;
	ace->sc_bio.databuf = va;
#ifndef ACE_DUMP_NOT_TRUSTED
	ace->active_xfer = bp;
	wakeup(&ace->ch_thread);

	switch(ace->sc_bio.error) {
	case ETIMEDOUT:
		printf("acedump: device timed out");
		err = EIO;
		break;
	case 0:
		err = 0;
		break;
	default:
		panic("acedump: unknown error type");
	}
	if (err != 0) {
		printf("\n");
		return err;
	}
#else	/* ACE_DUMP_NOT_TRUSTED */
	/* Let's just talk about this first... */
	device_printf(ace->sc_dev, ": dump addr 0x%p, size %zu blkno %llx\n",
	    va, size, blkno);
	DELAY(500 * 1000);	/* half a second */
	err = 0;
	__USE(err);
#endif

	acedoingadump = 0;
	return 0;
}

#ifdef HAS_BAD144_HANDLING
/*
 * Internalize the bad sector table.
 */
void
bad144intern(struct ace_softc *ace)
{
	struct dkbad *bt = &ace->sc_dk.dk_cpulabel->bad;
	struct disklabel *lp = ace->sc_dk.dk_label;
	int i = 0;

	DEBUG_PRINT(("bad144intern\n"), DEBUG_XFERS);

	for (; i < NBT_BAD; i++) {
		if (bt->bt_bad[i].bt_cyl == 0xffff)
			break;
		ace->sc_bio.badsect[i] =
		    bt->bt_bad[i].bt_cyl * lp->d_secpercyl +
		    (bt->bt_bad[i].bt_trksec >> 8) * lp->d_nsectors +
		    (bt->bt_bad[i].bt_trksec & 0xff);
	}
	for (; i < NBT_BAD+1; i++)
		ace->sc_bio.badsect[i] = -1;
}
#endif

static void
ace_set_geometry(struct ace_softc *ace)
{
	struct disk_geom *dg = &ace->sc_dk.dk_geom;

	memset(dg, 0, sizeof(*dg));

	dg->dg_secperunit = ace->sc_capacity;
	dg->dg_secsize = DEV_BSIZE /* XXX 512? */;
	dg->dg_nsectors = ace->sc_params.CurrentSectorsPerTrack;
	dg->dg_ntracks = ace->sc_params.CurrentNumberOfHeads;

	disk_set_info(ace->sc_dev, &ace->sc_dk, ST506);
}
