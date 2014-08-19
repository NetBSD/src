/*	$NetBSD: flash_ebus.c,v 1.4.6.3 2014/08/20 00:02:51 tls Exp $	*/

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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: flash_ebus.c,v 1.4.6.3 2014/08/20 00:02:51 tls Exp $");

/* Driver for the Intel 28F320/640/128 (J3A150) StrataFlash memory device
 * Extended to include the Intel JS28F256P30T95.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <uvm/uvm_extern.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/syslog.h>
#include <sys/vnode.h>
#include <sys/kthread.h>
#include <sys/lock.h>
#include <sys/queue.h>

#include <sys/rnd.h>

#include "locators.h"
#include <prop/proplib.h>

#include <emips/ebus/ebusvar.h>
#include <emips/emips/machdep.h>
#include <machine/emipsreg.h>

/* Internal config switches
 */
#define USE_BUFFERED_WRITES 0    /* Faster, but might not work in some (older) cases */
#define Verbose 0

/* Debug tools
 */
#define DEBUG_INTR   0x01
#define DEBUG_XFERS  0x02
#define DEBUG_STATUS 0x04
#define DEBUG_FUNCS  0x08
#define DEBUG_PROBE  0x10
#define DEBUG_WRITES 0x20
#define DEBUG_READS  0x40
#define DEBUG_ERRORS 0x80
#ifdef DEBUG
int eflash_debug = DEBUG_ERRORS;
#define EFLASH_DEBUG(x) (eflash_debug & (x))
#define DBGME(_lev_,_x_) if ((_lev_) & eflash_debug) _x_
#else
#define EFLASH_DEBUG(x) (0)
#define DBGME(_lev_,_x_)
#endif
#define DEBUG_PRINT(_args_,_lev_) DBGME(_lev_,printf _args_)

/* Product ID codes
 */
#define MANUF_INTEL  0x89
#define DEVICE_320   0x16
#define DEVICE_640   0x17
#define DEVICE_128   0x18
#define DEVICE_256   0x19

/* Table of chips we understand.
 */
#define nDELTAS 3
struct flash_type {
    struct {
        uint32_t nSectors;
        uint32_t nKB;
    } ft_deltas[nDELTAS];
    uint8_t ft_manuf_code;
    uint8_t ft_device_code;
    uint16_t ft_total_sectors;
    const char *ft_name;
};

static const struct flash_type sector_maps[] = {
    {
     {{32,128},{0,0},},
     MANUF_INTEL, DEVICE_320, 32,   /* a J3 part */
     "StrataFlash 28F320"
    },
    {
     {{64,128},{0,0},},
     MANUF_INTEL, DEVICE_640, 64,   /* a J3 part */
     "StrataFlash 28F640"
    },
    {
     {{128,128},{0,0},},
     MANUF_INTEL, DEVICE_128, 128,   /* a J3 part */
     "StrataFlash 28F128"
    },
    {
     {{255,128},{4,32},{0,0}},
     MANUF_INTEL, DEVICE_256, 259,  /* a P30 part */
	 "StrataFlash 28F256"
    }
};
#define nMAPS ((sizeof sector_maps) / (sizeof sector_maps[0]))

/* Instead of dragging in atavar.h.. */
struct eflash_bio {
	volatile int flags;/* cmd flags */
#define	ATA_POLL	0x0002	/* poll for completion */
#define	ATA_SINGLE	0x0008	/* transfer must be done in singlesector mode */
#define	ATA_READ	0x0020	/* transfer is a read (otherwise a write) */
#define	ATA_CORR	0x0040	/* transfer had a corrected error */
	daddr_t		blkno;	/* block addr */
	daddr_t		blkdone;/* number of blks transferred */
	size_t		nblks;	/* number of blocks currently transferring */
	size_t	    nbytes;	/* number of bytes currently transferring */
	char		*databuf;/* data buffer address */
	volatile int	error;
	u_int32_t	r_error;/* copy of status register */
#ifdef HAS_BAD144_HANDLING
	daddr_t		badsect[127];/* 126 plus trailing -1 marker */
#endif
};
/* End of atavar.h*/

/* chip-specific functions
 */
struct flash_ops;

/*
 * Device softc
 */
struct eflash_softc {
	device_t sc_dev;

	/* General disk infos */
	struct disk sc_dk;
	struct bufq_state *sc_q;
	struct callout sc_restart_ch;

	/* IDE disk soft states */
	struct buf *sc_bp; /* buf being transfered */
	struct buf *active_xfer; /* buf handoff to thread  */
	struct eflash_bio sc_bio; /* current transfer */

    struct proc *ch_thread;
    int ch_flags;
#define ATACH_SHUTDOWN 0x02        /* thread is shutting down */
#define ATACH_IRQ_WAIT 0x10        /* thread is waiting for irq */
#define ATACH_DISABLED 0x80        /* channel is disabled */
#define ATACH_TH_RUN   0x100       /* the kernel thread is working */
#define ATACH_TH_RESET 0x200       /* someone ask the thread to reset */

	int openings;
	int sc_flags;
#define	EFLASHF_WLABEL	0x004 /* label is writable */
#define	EFLASHF_LABELLING	0x008 /* writing label */
#define EFLASHF_LOADED	0x010 /* parameters loaded */
#define EFLASHF_WAIT	0x020 /* waiting for resources */
#define EFLASHF_KLABEL	0x080 /* retain label after 'full' close */

	int retries; /* number of xfer retry */

	krndsource_t	rnd_source;

    /* flash-specific state */
	struct _Flash *sc_dp;
    uint32_t sc_size;
    uint32_t sc_capacity;
    paddr_t  sc_base;
    volatile uint8_t *sc_page0;

    /* current read-write sector mapping */
    /*volatile*/ uint8_t *sc_sector;
    uint32_t sc_sector_size;
    uint32_t sc_sector_offset;
#define NOSECTOR ((uint32_t)(~0))
    int sc_erased;

    /* device-specificity */
    uint32_t sc_buffersize;
    vsize_t sc_max_secsize;
    unsigned int sc_chips;
    const struct flash_ops *sc_ops;
    struct flash_type sc_type;
};

static int	eflash_ebus_match (device_t, cfdata_t, void *);
static void	eflash_ebus_attach (device_t, device_t, void *);

CFATTACH_DECL_NEW(flash_ebus, sizeof (struct eflash_softc),
    eflash_ebus_match, eflash_ebus_attach, NULL, NULL);

/* implementation decls */
static int flash_identify(struct eflash_softc*);
static int KBinSector(struct flash_type * SecMap, unsigned int SecNo);
static uint32_t SectorStart(struct flash_type * SecMap, int SecNo);
static unsigned int SectorNumber(struct flash_type * SecMap, uint32_t Offset);
static void eflash_thread(void *arg);
static int eflash_read_at (struct eflash_softc *sc, daddr_t start_sector, char *buffer,
                           size_t nblocks, size_t * pSizeRead);
static int eflash_write_at(struct eflash_softc *sc, daddr_t start_sector, char *buffer,
                           size_t nblocks, size_t * pSizeWritten);

/* Config functions
 */
static int
eflash_ebus_match(device_t parent, cfdata_t match, void *aux)
{
	struct ebus_attach_args *ia = aux;
	struct _Flash *f = (struct _Flash *)ia->ia_vaddr;

	if (strcmp("flash", ia->ia_name) != 0)
		return (0);
	if ((f == NULL) ||
	    ((f->BaseAddressAndTag & FLASHBT_TAG) != PMTTAG_FLASH))
		return (0);

	return (1);
}

static void
eflash_ebus_attach(device_t parent, device_t self, void *aux)
{
	struct ebus_attach_args *ia =aux;
	struct eflash_softc *sc = device_private(self);
    uint32_t base, ctrl;
    int error;

    /* Plan.
     * - mips_map_physmem() (with uncached) first page
     * - keep it around since we need status ops
     * - find what type it is.
     * - then mips_map_physmem() each sector as needed.
     */

	sc->sc_dev = self;
	sc->sc_dp = (struct _Flash*)ia->ia_vaddr;
    base = sc->sc_dp->BaseAddressAndTag & FLASHBT_BASE;
    ctrl = sc->sc_dp->Control;

    sc->sc_size = ctrl & FLASHST_SIZE;
    sc->sc_capacity = sc->sc_size / DEV_BSIZE;
    sc->sc_base = base;
    /* The chip is 16bit, so if we get 32bit there are two */
    sc->sc_chips = (ctrl & FLASHST_BUS_32) ? 2 : 1;

    /* Map the first page to see what chip we got */
    sc->sc_page0 = (volatile uint8_t *) mips_map_physmem(base, PAGE_SIZE);

    if (flash_identify(sc)) {
        printf(" base %x: %dMB flash memory (%d x %s)\n", base, sc->sc_size >> 20,
               sc->sc_chips, sc->sc_type.ft_name);
    } else {
        /* BUGBUG If we dont identify it stop the driver! */
        printf(": unknown manufacturer id %x, device id %x\n",
               sc->sc_type.ft_manuf_code, sc->sc_type.ft_device_code);
    }

    config_pending_incr(self);

	error = kthread_create(PRI_NONE, 0, NULL,
	    eflash_thread, sc, NULL, "%s", device_xname(sc->sc_dev));
	if (error)
		aprint_error_dev(sc->sc_dev,
		    "unable to create kernel thread: error %d\n", error);
}

/* Implementation functions
 */
/* Returns the size in KBytes of a given sector,
 * or -1 for bad arguments.
 */
static int KBinSector(struct flash_type * SecMap, unsigned int SecNo)
{
    int i;

    for (i = 0; i < nDELTAS; i++) {
        if (SecNo < SecMap->ft_deltas[i].nSectors)
            return SecMap->ft_deltas[i].nKB;
        SecNo -= SecMap->ft_deltas[i].nSectors;
    }

    return -1;
}

#define SectorSize(_map_,_sector_) (1024 * KBinSector(_map_,_sector_))

/* Whats the starting offset of sector N
 */
static uint32_t SectorStart(struct flash_type * SecMap, int SecNo)
{
    int i;
    uint32_t Offset = 0;

    for (i = 0; i < nDELTAS; i++) {
        if ((unsigned int)SecNo < SecMap->ft_deltas[i].nSectors)
            return 1024 * (Offset + (SecMap->ft_deltas[i].nKB * SecNo));
        SecNo -= SecMap->ft_deltas[i].nSectors;
        Offset += SecMap->ft_deltas[i].nSectors * SecMap->ft_deltas[i].nKB;
    }

    return ~0;
}

/* What sector number corresponds to a given offset
 */
static unsigned int SectorNumber(struct flash_type * SecMap, uint32_t Offset)
{
    unsigned int i;
    unsigned int SecNo = 0;

    Offset /= 1024;
    for (i = 0; i < nDELTAS; i++) {
        if (Offset < (unsigned int)
            ((SecMap->ft_deltas[i].nSectors * SecMap->ft_deltas[i].nKB)))
            return SecNo + (Offset / SecMap->ft_deltas[i].nKB);
        SecNo += SecMap->ft_deltas[i].nSectors;
        Offset -= SecMap->ft_deltas[i].nSectors * SecMap->ft_deltas[i].nKB;
    }

    return ~0;
}

/*
 * Semi-generic operations
 */
struct flash_ops {
    void (*write_uint8)    (struct eflash_softc *sc, volatile void *Offset, uint8_t Value);
    void (*read_uint8)     (struct eflash_softc *sc, volatile void *Offset, uint8_t *Value);
    void (*write_uint16)   (struct eflash_softc *sc, volatile void *Offset, uint16_t Value);
    void (*read_uint16)    (struct eflash_softc *sc, volatile void *Offset, uint16_t *Value);
    void (*write_uint32)   (struct eflash_softc *sc, volatile void *Offset, uint32_t Value);
    void (*read_uint32)    (struct eflash_softc *sc, volatile void *Offset, uint32_t *Value);
    int  (*program_word)   (struct eflash_softc *sc, volatile void *Offset, uint16_t *pValues,
                            int  Verify, int *nWritten);
    int  (*program_buffer) (struct eflash_softc *sc, volatile void *Offset, uint16_t *pValues,
                            int  Verify, int *nWritten);
};

/*
 * Hardware access proper, single-chip
 */
static void single_write_uint8  (struct eflash_softc *sc,volatile void *Offset,uint8_t Value)
{
    volatile uint8_t * Where = Offset;
    *Where = Value;
}

static void single_read_uint8   (struct eflash_softc *sc,volatile void *Offset,uint8_t *Value)
{
    volatile uint8_t * Where = Offset;
    *Value = *Where;
}

static void single_write_uint16 (struct eflash_softc *sc,volatile void *Offset,uint16_t Value)
{
    volatile uint16_t * Where = Offset;
    *Where = Value;
}

static void single_read_uint16  (struct eflash_softc *sc,volatile void *Offset,uint16_t *Value)
{
    volatile uint16_t * Where = Offset;
    *Value = *Where;
}

/* This one should not be used, probably */
static void single_write_uint32 (struct eflash_softc *sc,volatile void *Offset,uint32_t Value)
{
#if 0
    /* The chip cannot take back-to-back writes */
    volatile uint32_t * Where = Offset;
    *Where = Value;
#else
    volatile uint8_t * Where = Offset;
    uint16_t v0, v1;

    /* Unfortunately, this is bytesex dependent */
#if (BYTE_ORDER == BIG_ENDIAN)
    v1 = (uint16_t) Value;
    v0 = (uint16_t) (Value >> 16);
#else
    v0 = (uint16_t) Value;
    v1 = (uint16_t) (Value >> 16);
#endif
    single_write_uint16(sc,Where,v0);
    single_write_uint16(sc,Where+2,v1);
#endif
}

static void single_read_uint32  (struct eflash_softc *sc,volatile void *Offset,uint32_t *Value)
{
    /* back-to-back reads must be ok */
    volatile uint32_t * Where = Offset;
    *Value = *Where;
}

/*
 * Hardware access proper, paired-chips
 * NB: This set of ops assumes two chips in parallel on a 32bit bus,
 *     each operation is repeated in parallel to both chips
 */
static void twin_write_uint8  (struct eflash_softc *sc,volatile void *Offset,uint8_t Value)
{
    volatile uint32_t * Where = Offset;
    uint32_t v = Value | ((uint32_t)Value << 16);

    v = le32toh(v);
    *Where = v;
}

static void twin_read_uint8   (struct eflash_softc *sc,volatile void *Offset,uint8_t *Value)
{
    volatile uint32_t * Where = Offset;
    uint32_t v;
    v = *Where;
    v = le32toh(v);
    *Value = (uint8_t) v;
}

/* This one should *not* be used, error-prone */
static void twin_write_uint16 (struct eflash_softc *sc,volatile void *Offset,uint16_t Value)
{
    volatile uint16_t * Where = Offset;
    *Where = Value;
}

static void twin_read_uint16  (struct eflash_softc *sc,volatile void *Offset,uint16_t *Value)
{
    volatile uint16_t * Where = Offset;
    *Value = *Where;
}

static void twin_write_uint32 (struct eflash_softc *sc,volatile void *Offset,uint32_t Value)
{
    volatile uint32_t * Where = Offset;
    Value = le32toh(Value);
    *Where = Value;
}

static void twin_read_uint32  (struct eflash_softc *sc,volatile void *Offset,uint32_t *Value)
{
    volatile uint32_t * Where = Offset;
    uint32_t v;
    v = *Where;
    v = le32toh(v);
    *Value = v;
}

/*
 * Command and status definitions
 */

/* Defines for the STATUS register
 */
#define ST_reserved          0x01
#define ST_BLOCK_LOCKED      0x02
#define ST_PROGRAM_SUSPENDED 0x04
#define ST_LOW_VOLTAGE       0x08
#define ST_LOCK_BIT_ERROR    0x10
#define ST_ERASE_ERROR       0x20
#define ST_ERASE_SUSPENDED   0x40
#define ST_READY             0x80
#define ST_ERASE_MASK        0xee  /* bits to check after erase command */
#define ST_MASK              0xfe  /* ignore reserved */

/* Command set (what we use of it)
 */
#define CMD_CONFIRM       0xd0
#define CMD_READ_ARRAY    0xff
#define CMD_READ_ID       0x90
#define CMD_READ_STATUS   0x70
#define CMD_CLEAR_STATUS  0x50
#define CMD_WRITE_WORD    0x40
#define CMD_WRITE_BUFFER  0xe8
#define CMD_ERASE_SETUP   0x20
#define CMD_ERASE_CONFIRM CMD_CONFIRM
#define CMD_SET_PREFIX    0x60  /* set read config, lock bits */
#define CMD_LOCK          0x01
#define CMD_UNLOCK        CMD_CONFIRM
/* What we dont use of it
 */
#define CMD_READ_QUERY    0x98
# define BUFFER_BYTES          32
#define CMD_ERASE_SUSPEND 0xb0
#define CMD_ERASE_RESUME  CMD_CONFIRM
#define CMD_CONFIGURATION 0xb8
#define CMD_PROTECT       0xc0

/* Enter the Product ID mode (Read Identifier Codes)
 */
static void ProductIdEnter(struct eflash_softc *sc)
{
    sc->sc_ops->write_uint8(sc,sc->sc_page0,CMD_READ_ID);
}

/* Exit the Product ID mode (enter Read Array mode)
 */
static void ProductIdExit(struct eflash_softc *sc)
{
    sc->sc_ops->write_uint8(sc,sc->sc_page0,CMD_READ_ARRAY);
}

/* Read the status register
 */
static uint8_t ReadStatusRegister(struct eflash_softc *sc)
{
    uint8_t Status;

    sc->sc_ops->write_uint8(sc,sc->sc_page0,CMD_READ_STATUS);
    sc->sc_ops->read_uint8(sc,sc->sc_page0,&Status);
    sc->sc_ops->write_uint8(sc,sc->sc_page0,CMD_READ_ARRAY);
    return Status;
}

/* Clear error bits in status
 */
static void ClearStatusRegister(struct eflash_softc *sc)
{
    sc->sc_ops->write_uint8(sc,sc->sc_page0,CMD_CLEAR_STATUS);
}

#if DEBUG
/* Decode status bits
 */
typedef const char *string;

static void PrintStatus(uint8_t Status)
{
    /* BUGBUG there's a %b format I think? */
    string BitNames[8] = {
        "reserved", "BLOCK_LOCKED",
        "PROGRAM_SUSPENDED", "LOW_VOLTAGE",
        "LOCK_BIT_ERROR", "ERASE_ERROR",
        "ERASE_SUSPENDED", "READY"
    };
    int i;
    int  OneSet = FALSE;

    printf("[status %x =",Status);
    for (i = 0; i < 8; i++) {
        if (Status & (1<<i)) {
            printf("%c%s", 
                     (OneSet) ? '|' : ' ',
                     BitNames[i]);
            OneSet = TRUE;
        }
    }
    printf("]\n");
}
#else
#define PrintStatus(x)
#endif

/* 
 * The device can lock up under certain conditions.
 * There is no software workaround [must toggle RP# to GND]
 * Check if it seems that we are in that state.
 */
static int  IsIrresponsive(struct eflash_softc *sc)
{
    uint8_t Status = ReadStatusRegister(sc);

    if (Status & ST_READY)
        return FALSE;

    if ((Status & ST_ERASE_MASK) == 
        (ST_LOCK_BIT_ERROR|ST_ERASE_SUSPENDED|ST_ERASE_ERROR)) {
        /* yes, looks that way */
        return TRUE;
    }

    /* Something is indeed amiss, but we dont really know for sure */
    PrintStatus(ReadStatusRegister(sc));
    ClearStatusRegister(sc);
    PrintStatus(ReadStatusRegister(sc));

    if ((Status & ST_MASK) == 
        (ST_LOCK_BIT_ERROR|ST_ERASE_SUSPENDED|ST_ERASE_ERROR)) {
        /* yes, looks that way */
        return TRUE;
    }

    return FALSE;
}


/* Write one 16bit word
 */
static int 
single_program_word(struct eflash_softc *sc, volatile void *Offset, uint16_t *Values,
                  int  Verify, int *nWritten)
{
    uint8_t Status;
    uint16_t i, Data16, Value;

    *nWritten = 0;

    Value = Values[0];

    if (Verify) {
        sc->sc_ops->read_uint16(sc,Offset,&Data16);
#ifdef Verbose
        if (Verbose) {
            printf("Location %p was x%x\n", 
                   Offset, Data16);
        }
#endif
        if (Data16 != 0xffff)
            printf("Offset %p not ERASED, wont take.\n",Offset);
    }

    sc->sc_ops->write_uint8(sc,sc->sc_page0,CMD_WRITE_WORD);
    sc->sc_ops->write_uint16(sc,Offset,Value);

    /* Wait until the operation is completed
     * Specs say it takes between 210 and 630 us
     * Errata says 360 TYP and Max=TBD (sic)
     */
    DELAY(800);

    for (i = 0; i < 10; i++) {
        sc->sc_ops->read_uint8(sc,Offset,&Status);
        if ((Status & ST_READY)) break;
        DELAY(100);
    }

    ProductIdExit(sc);

    if (Verify) {
        sc->sc_ops->read_uint16(sc,Offset,&Data16);
#ifdef Verbose
        if (Verbose) {
            printf("Location %p is now x%x\n", 
                   Offset, Data16);
        }
#endif
        if ((Data16 != Value)) {
            PrintStatus(Status);
            printf(". That didnt work, try again.. [%x != %x]\n", 
                   Data16, Value);
            ClearStatusRegister(sc);
            return FALSE;
        }
    }

    *nWritten = 2;
    return TRUE;
}

/* Write one buffer, 16bit words at a time
 */
static int 
single_program_buffer(struct eflash_softc *sc, volatile void *Offset, uint16_t *Values,
                  int  Verify, int *nWritten)
{
    uint8_t Status;
    uint16_t i, Data16, Value = 0;
    volatile uint8_t *Where = Offset;

    *nWritten = 0;
    if (sc->sc_buffersize == 0)
        return FALSE; /* sanity */

    if (Verify) {
        for (i = 0; i < sc->sc_buffersize; i+= 2) {
            sc->sc_ops->read_uint16(sc,Where+i,&Data16);
#ifdef Verbose
            if (Verbose) {
                printf("Location %p was x%x\n", 
                       Where+i, Data16);
            }
#endif

            if (Data16 != 0xffff)
                printf("Offset %p not ERASED, wont take.\n",Where+i);
        }
    }

    /* Specs say to retry if necessary */
    for (i = 0; i < 5; i++) {
        sc->sc_ops->write_uint8(sc,Offset,CMD_WRITE_BUFFER);
        DELAY(10);
        sc->sc_ops->read_uint8(sc,Offset,&Status);
        if ((Status & ST_READY)) break;
    }
    if (0 == (Status & ST_READY)) {
        printf("FAILED program_buffer at Location %p, Status= x%x\n", 
                 Offset, Status);
        return FALSE;
    }

    /* Say how many words we'll be sending */
    sc->sc_ops->write_uint8(sc,Offset,(uint8_t)(sc->sc_buffersize/2));

    /* Send the data */
    for (i = 0; i < sc->sc_buffersize; i+= 2) {
        Value = Values[i/2];
        sc->sc_ops->write_uint16(sc,Where+i,Value);
        DELAY(10);/*jic*/
    }

    /* Write confirmation */
    sc->sc_ops->write_uint8(sc,Offset,CMD_CONFIRM);

    /* Wait until the operation is completed
     * Specs say it takes between 800 and 2400 us
     * Errata says 1600 TYP and Max=TBD (sic), but fixed in stepping A3 and above.
     */
    DELAY(800);

    for (i = 0; i < 20; i++) {
        sc->sc_ops->write_uint8(sc,Offset,CMD_READ_STATUS);
        sc->sc_ops->read_uint8(sc,Offset,&Status);
        if ((Status & ST_READY)) break;
        DELAY(200);
    }

    ProductIdExit(sc);

    /* Verify? */
    if (Verify) {
        for (i = 0; i < sc->sc_buffersize; i+= 2) {
            sc->sc_ops->read_uint16(sc,Where+i,&Data16);
#ifdef Verbose
            if (Verbose) {
                printf("Location %p is now x%x\n", 
                       Where+i, Data16);
            }
#endif             
            Value = Values[i/2];

            if ((Data16 != Value)) {
                PrintStatus(Status);
                printf(". That didnt work, try again.. [%x != %x]\n", 
                       Data16, Value);
                ClearStatusRegister(sc);
                return FALSE;
            }
        }
    }

    *nWritten = sc->sc_buffersize;
    return TRUE;
}

/* Write one 32bit word
 */
static int 
twin_program_word(struct eflash_softc *sc, volatile void *Offset, uint16_t *Values,
                int  Verify, int *nWritten)
{
    uint8_t Status;
    uint32_t i, Data32, Value;
    uint16_t v0, v1;

    *nWritten = 0;

    v0 = Values[0];
    v0 = le16toh(v0);
    v1 = Values[1];
    v1 = le16toh(v1);
    Value = v0 | ((uint32_t)v1 << 16);
    if (Verify) {
        sc->sc_ops->read_uint32(sc,Offset,&Data32);
#ifdef Verbose
        if (Verbose) {
            printf("Location %p was x%x\n", 
                   Offset, Data32);
        }
#endif
        if (Data32 != 0xffffffff)
            printf("Offset %p not ERASED, wont take.\n",Offset);
    }

    sc->sc_ops->write_uint8(sc,sc->sc_page0,CMD_WRITE_WORD);
    sc->sc_ops->write_uint32(sc,Offset,Value);

    /* Wait until the operation is completed
     * Specs say it takes between 210 and 630 us
     * Errata says 360 TYP and Max=TBD (sic)
     */
    DELAY(400);

    for (i = 0; i < 10; i++) {
        sc->sc_ops->read_uint8(sc,Offset,&Status);
        if ((Status & ST_READY)) break;
        DELAY(100);
    }

    ProductIdExit(sc);

    if (Verify) {
        sc->sc_ops->read_uint32(sc,Offset,&Data32);
#ifdef Verbose
        if (Verbose) {
            printf("Location %p is now x%x\n", 
                   Offset, Data32);
        }
#endif             
        if ((Data32 != Value)) {
            PrintStatus(Status);
            printf(". That didnt work, try again.. [%x != %x]\n", 
                   Data32, Value);
            ClearStatusRegister(sc);
            return FALSE;
        }
    }

    *nWritten = 4;
    return TRUE;
}

/* Write one buffer, 32bit words at a time
 */
static int 
twin_program_buffer(struct eflash_softc *sc, volatile void *Offset, uint16_t *Values,
                int  Verify, int *nWritten)
{
    uint8_t Status;
    uint32_t i, Data32, Value;
    uint16_t v0 = 0, v1;
    volatile uint8_t *Where = Offset;

    *nWritten = 0;
    if (sc->sc_buffersize == 0)
        return FALSE; /* sanity */

    if (Verify) {
        for (i = 0; i < sc->sc_buffersize; i+= 4) {
            sc->sc_ops->read_uint32(sc,Where+i,&Data32);
#ifdef Verbose
            if (Verbose) {
                printf("Location %p was x%x\n", 
                       Where+i, Data32);
            }
#endif
            if (Data32 != 0xffffffff)
                printf("Offset %p not ERASED, wont take.\n",Where+i);
        }
    }

    /* Specs say to retry if necessary */
    for (i = 0; i < 5; i++) {
        sc->sc_ops->write_uint8(sc,Offset,CMD_WRITE_BUFFER);
        DELAY(10);
        sc->sc_ops->read_uint8(sc,Offset,&Status);
        if ((Status & ST_READY)) break;
    }
    if (0 == (Status & ST_READY)) {
        printf("FAILED program_buffer at Location %p, Status= x%x\n", 
                 Offset, Status);
        return FALSE;
    }

    /* Say how many words we'll be sending */
    sc->sc_ops->write_uint8(sc,Offset,(uint8_t)(sc->sc_buffersize/4)); /* to each twin! */

    /* Send the data */
    for (i = 0; i < sc->sc_buffersize; i+= 4) {
        v0 = Values[i/2];
        v0 = le16toh(v0);
        v1 = Values[1+(i/2)];
        v1 = le16toh(v1);
        Value = v0 | ((uint32_t)v1 << 16);
        sc->sc_ops->write_uint32(sc,Where+i,Value);
        DELAY(10);/*jic*/
    }

    /* Write confirmation */
    sc->sc_ops->write_uint8(sc,Offset,CMD_CONFIRM);

    /* Wait until the operation is completed
     * Specs say it takes between 800 and 2400 us
     * Errata says 1600 TYP and Max=TBD (sic), but fixed in stepping A3 and above.
     */
    DELAY(800);

    for (i = 0; i < 20; i++) {
        sc->sc_ops->write_uint8(sc,Offset,CMD_READ_STATUS);
        sc->sc_ops->read_uint8(sc,Offset,&Status);
        if ((Status & ST_READY)) break;
        DELAY(200);
    }

    ProductIdExit(sc);

    /* Verify */
    if (Verify) {
        for (i = 0; i < sc->sc_buffersize; i+= 4) {
            sc->sc_ops->read_uint32(sc,Where+i,&Data32);
#ifdef Verbose
            if (Verbose) {
                printf("Location %p is now x%x\n", 
                       Where+i, Data32);
            }
#endif
            v0 = Values[i/2];
            v0 = le16toh(v0);
            v1 = Values[1+(i/2)];
            v1 = le16toh(v1);
            Value = v0 | ((uint32_t)v1 << 16);

            if ((Data32 != Value)) {
                PrintStatus(Status);
                printf(". That didnt work, try again.. [%x != %x]\n", 
                       Data32, Value);
                ClearStatusRegister(sc);
                return FALSE;
            }
        }
    }

    *nWritten = sc->sc_buffersize;
    return TRUE;
}

/* Is there a lock on a given sector
 */
static int IsSectorLocked(struct eflash_softc *sc, uint8_t *secptr)
{
    uint8_t Data, Data1;

    ProductIdEnter(sc);
    /* Lockout info is at address 2 of the given sector, meaning A0=0 A1=1.
     */
    sc->sc_ops->read_uint8(sc,secptr+(0x0002*2*sc->sc_chips),&Data);
    sc->sc_ops->read_uint8(sc,secptr+(0x0003*2*sc->sc_chips),&Data1);

    ProductIdExit(sc);

    return (Data & 1);
}

/* Remove the write-lock to a sector
 */
static void SectorUnLock(struct eflash_softc *sc, uint8_t *secptr)
{
    uint8_t Status;
    int i;

    DBGME(DEBUG_FUNCS,printf("%s: Unlocking sector %d [ptr %p] ...\n",
	device_xname(sc->sc_dev), sc->sc_sector_offset, secptr));

    sc->sc_ops->write_uint8(sc,sc->sc_page0,CMD_SET_PREFIX);
    sc->sc_ops->write_uint8(sc,secptr,CMD_UNLOCK);

    /* Wait until the unlock is complete.
     * Specs say this takes between 64 and 75 usecs.
     */
    DELAY(100);

    for (i = 0; i < 10; i++) {
        sc->sc_ops->read_uint8(sc,secptr,&Status);
        if ((Status & ST_READY)) break;
        DELAY(100);
    }

    ProductIdExit(sc);

    if ((Status & ST_MASK) == ST_READY) {
        DBGME(DEBUG_FUNCS,printf("%s: Unlocked ok.\n",
	    device_xname(sc->sc_dev)));
        return;
    }

    PrintStatus(Status);
    DBGME(DEBUG_ERRORS,printf("%s: Unlock of sector %d NOT completed (status=%x).\n",
                              device_xname(sc->sc_dev),
			      sc->sc_sector_offset, Status));
    ClearStatusRegister(sc);
}


/* Erase one sector
 */
static int  SectorErase(struct eflash_softc *sc, void *secptr)
{
    uint8_t Status = 0;
    uint16_t i;

    DBGME(DEBUG_FUNCS,printf("%s: Erasing sector %d [ptr %p] ...\n",
	device_xname(sc->sc_dev), sc->sc_sector_offset, secptr));

    /* On some chips we just cannot avoid the locking business.
     */
    if ((sc->sc_chips == 1) &&
        IsSectorLocked(sc,secptr))
        SectorUnLock(sc,secptr);

    sc->sc_ops->write_uint8(sc,secptr,CMD_ERASE_SETUP);
    sc->sc_ops->write_uint8(sc,secptr,CMD_ERASE_CONFIRM);

    /* Wait until the erase is actually completed
     * Specs say it will take between 1 and 5 seconds.
     * Errata says it takes 2 sec min and 25 sec max.
     * Double that before giving up.
     */
    for (i = 0; i < 20; i++) {
        /* Sleep for at least 2 seconds
         */
        tsleep(sc,PWAIT,"erase", hz * 2);

        sc->sc_ops->read_uint8(sc,secptr,&Status);
        if ((Status & ST_READY)) break;
        PrintStatus(Status);
    }

    ProductIdExit(sc);

    if ((Status & ST_ERASE_MASK) == ST_READY) {
        DBGME(DEBUG_FUNCS,printf("%s: Erased ok.\n", device_xname(sc->sc_dev)));
        return 0;
    }

    PrintStatus(Status);
    DBGME(DEBUG_ERRORS,printf("%s: Erase of sector %d NOT completed (status=%x).\n",
                              device_xname(sc->sc_dev),
			      sc->sc_sector_offset, Status));

    ClearStatusRegister(sc);
    return EIO;
}



/* Write (a portion of) a sector
 */
static size_t eflash_write_sector(struct eflash_softc *sc, char *Buffer, size_t n,
                               uint8_t *Offset, int Verify)
{
    size_t i;

    /* Make sure the device is not screwed up
     */
    if (IsIrresponsive(sc)) {
        printf("FLASH is locked-up (or mapped cacheable?), wont work. ");
    }

    for (i = 0; i < n;) {
        int nTries;
        int nWritten = 0;/*we expect 2 or 4 */

        if (sc->sc_buffersize && ((n-i) >= sc->sc_buffersize)) {
            for (nTries = 0; nTries < 5; nTries++)
                if (sc->sc_ops->program_buffer(sc,Offset,(uint16_t*)(Buffer+i),Verify,&nWritten))
                    break;
        } else {
            for (nTries = 0; nTries < 5; nTries++)
                if (sc->sc_ops->program_word(sc,Offset,(uint16_t*)(Buffer+i),Verify,&nWritten))
                    break;
        }
        Offset += nWritten;
        i += nWritten;
        if (nWritten == 0)
            break;
    }
    return i;
}

/* Identify type and the sector map of the FLASH.
 * Argument is the base address of the device and the count of chips on the bus (1/2)
 * Returns FALSE if failed
 */
static const struct flash_ops single_ops = {
    single_write_uint8,
    single_read_uint8,
    single_write_uint16,
    single_read_uint16,
    single_write_uint32,
    single_read_uint32,
    single_program_word,
    single_program_buffer
};

static const struct flash_ops twin_ops = {
    twin_write_uint8,
    twin_read_uint8,
    twin_write_uint16,
    twin_read_uint16,
    twin_write_uint32,
    twin_read_uint32,
    twin_program_word,
    twin_program_buffer
};

static int  flash_identify(struct eflash_softc *sc)
{
    uint8_t Mid, Did;
    int i;

    if (sc->sc_chips > 1)
        sc->sc_ops = &twin_ops;
    else
        sc->sc_ops = &single_ops;

    sc->sc_buffersize = 0;
#if USE_BUFFERED_WRITES
    sc->sc_buffersize = BUFFER_BYTES * sc->sc_chips;
#endif
    sc->sc_sector = NULL;
    sc->sc_sector_size = 0;
    sc->sc_sector_offset = NOSECTOR;
    sc->sc_erased = FALSE;

    ProductIdEnter(sc);
    sc->sc_ops->read_uint8(sc,sc->sc_page0+(0x0000*2*sc->sc_chips),&Mid);
    sc->sc_ops->read_uint8(sc,sc->sc_page0+(0x0001*2*sc->sc_chips),&Did);
    ProductIdExit(sc);

    sc->sc_type.ft_manuf_code = Mid;
    sc->sc_type.ft_device_code = Did;

    for (i = 0; i < nMAPS; i++) {
        if ((sector_maps[i].ft_manuf_code == Mid) && (sector_maps[i].ft_device_code == Did)) {
            int j;
            uint32_t ms = 0;
            sc->sc_type = sector_maps[i];
            /* double the sector sizes if twin-chips */
            for (j = 0; j < nDELTAS; j++) {
                sc->sc_type.ft_deltas[j].nKB *= sc->sc_chips;
                if (ms < sc->sc_type.ft_deltas[j].nKB)
                    ms = sc->sc_type.ft_deltas[j].nKB;
            }
            sc->sc_max_secsize = ms * 1024;
            return TRUE;
        }
    }

    return FALSE;
}

/* Common code for read&write argument validation
 */
static int eflash_validate(struct eflash_softc *sc, daddr_t start, size_t *pSize, void **pSrc)
{
    daddr_t Size;
    uint32_t sec;
    size_t secsize, secstart;

    /* Validate args
     */
    if (start >= sc->sc_capacity) {
        *pSize = 0;
        DBGME(DEBUG_ERRORS,printf("eflash::ValidateArg(%qx) EOF\n", start));
        return E2BIG;
    }

    /* Map sector if not already
     */
    sec = SectorNumber(&sc->sc_type, start << DEV_BSHIFT);
    secsize = SectorSize( &sc->sc_type, sec);
    secstart = SectorStart(&sc->sc_type,sec);
    if (sec != sc->sc_sector_offset) {
        int error;

        /* unmap previous first */
        if (sc->sc_sector_offset != NOSECTOR) {
            DBGME(DEBUG_FUNCS,printf("%s: unmap %p %zx\n",
		device_xname(sc->sc_dev), sc->sc_sector, sc->sc_sector_size));
            iounaccess((vaddr_t)sc->sc_sector, sc->sc_sector_size);
            sc->sc_sector_offset = NOSECTOR;
        }

        /* map new */
        error = ioaccess((vaddr_t)sc->sc_sector,
                         secstart + sc->sc_base,
                         secsize);   
        DBGME(DEBUG_FUNCS,printf("%s: mapped %p %zx -> %zx %d\n",
	    device_xname(sc->sc_dev),
	    sc->sc_sector, secsize, secstart + sc->sc_base,error));
        if (error) return error;

        /* Update state. We have to assume the sector was not erased. Sigh. */
        sc->sc_sector_offset = sec;
        sc->sc_sector_size = secsize;
        sc->sc_erased = FALSE;
    }

    /* Adjust size if necessary
     */
    Size = start + *pSize; /* last sector */
    if (Size > sc->sc_capacity) {
        /* At most this many sectors 
         */
        Size = sc->sc_capacity - start;
        *pSize = (size_t)Size;
    }
    if (*pSize > (secsize >> DEV_BSHIFT)) {
        *pSize = secsize >> DEV_BSHIFT;
    }

    *pSrc = sc->sc_sector + (start << DEV_BSHIFT) - secstart;

    DBGME(DEBUG_FUNCS,printf("%s: Validate %qx %zd %p\n",
	device_xname(sc->sc_dev), start,*pSize, *pSrc));
    return 0;
}

static int eflash_read_at (struct eflash_softc *sc,
                           daddr_t start_sector, char *buffer, size_t nblocks,
                           size_t * pSizeRead)
{
    int error;
    uint32_t SizeRead = 0;
    void *src;

    DBGME(DEBUG_XFERS|DEBUG_READS,printf("%s: EflashReadAt(%qx %p %zd %p)\n",
                     device_xname(sc->sc_dev), start_sector, buffer, nblocks, pSizeRead));

    /* Validate & trim arguments
     */
    error = eflash_validate(sc, start_sector, &nblocks, &src);

    /* Copy data if
     */
    if (error == 0) {
        SizeRead = nblocks;
        memcpy(buffer, src, nblocks << DEV_BSHIFT);
    }

    if (pSizeRead)
        *pSizeRead = SizeRead;
    return error;
}

/* Write SIZE bytes to device.
 */
static int eflash_write_at (struct eflash_softc *sc,
                           daddr_t start_sector, char *buffer, size_t nblocks,
                           size_t * pSizeWritten)
{
    int error;
    void *src;
    size_t SizeWritten = 0;

    DBGME(DEBUG_XFERS|DEBUG_WRITES,printf("%s: EflashWriteAt(%qx %p %zd %p)\n",
                     device_xname(sc->sc_dev), start_sector, buffer, nblocks, pSizeWritten));

    /* Validate & trim arguments
     */
    error = eflash_validate(sc, start_sector, &nblocks, &src);

    if (error == 0) {
        /* Do we have to erase it */
        if (! sc->sc_erased) {

            error = SectorErase(sc,src);
            if (error)
                goto Out;
            sc->sc_erased = TRUE;
        }
        SizeWritten = eflash_write_sector(sc, buffer, nblocks << DEV_BSHIFT, src, TRUE);
        SizeWritten >>= DEV_BSHIFT;
    }

 Out:
    if (pSizeWritten)
        *pSizeWritten = SizeWritten;
    return error;
}

/* Rest of code lifted with mods from the dev\ata\wd.c driver
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

#define	EFLASHIORETRIES_SINGLE 4	/* number of retries before single-sector */
#define	EFLASHIORETRIES	5	/* number of retries before giving up */
#define	RECOVERYTIME hz/2	/* time to wait before retrying a cmd */

#define	EFLASHUNIT(dev)		DISKUNIT(dev)
#define	EFLASHPART(dev)		DISKPART(dev)
#define	EFLASHMINOR(unit, part)	DISKMINOR(unit, part)
#define	MAKEEFLASHDEV(maj, unit, part)	MAKEDISKDEV(maj, unit, part)

#define	EFLASHLABELDEV(dev)	(MAKEEFLASHDEV(major(dev), EFLASHUNIT(dev), RAW_PART))

void	eflashperror(const struct eflash_softc *);

extern struct cfdriver eflash_cd;

dev_type_open(eflashopen);
dev_type_close(eflashclose);
dev_type_read(eflashread);
dev_type_write(eflashwrite);
dev_type_ioctl(eflashioctl);
dev_type_strategy(eflashstrategy);
dev_type_dump(eflashdump);
dev_type_size(eflashsize);

const struct bdevsw eflash_bdevsw = {
	.d_open = eflashopen,
	.d_close = eflashclose,
	.d_strategy = eflashstrategy,
	.d_ioctl = eflashioctl,
	.d_dump = eflashdump,
	.d_psize = eflashsize,
	.d_discard = nodiscard,
	.d_flag = D_DISK
};

const struct cdevsw eflash_cdevsw = {
	.d_open = eflashopen,
	.d_close = eflashclose,
	.d_read = eflashread,
	.d_write = eflashwrite,
	.d_ioctl = eflashioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_DISK
};

void  eflashgetdefaultlabel(struct eflash_softc *, struct disklabel *);
void  eflashgetdisklabel(struct eflash_softc *);
void  eflashstart(void *);
void  __eflashstart(struct eflash_softc *, struct buf *);
void  eflashrestart(void *);
void  eflashattach(struct eflash_softc *);
int   eflashdetach(device_t, int);
int   eflashactivate(device_t, enum devact);

void  eflashdone(struct eflash_softc *);
static void eflash_set_geometry(struct eflash_softc *sc);

struct dkdriver eflashdkdriver = { eflashstrategy, minphys };

#ifdef HAS_BAD144_HANDLING
static void bad144intern(struct eflash_softc *);
#endif

static void eflash_wedges(void *arg);

void
eflashattach(struct eflash_softc *sc)
{
	device_t self = sc->sc_dev;
	char pbuf[9];
	DEBUG_PRINT(("%s: eflashattach\n",  device_xname(sc->sc_dev)), DEBUG_FUNCS | DEBUG_PROBE);

	callout_init(&sc->sc_restart_ch, 0);
	bufq_alloc(&sc->sc_q, BUFQ_DISK_DEFAULT_STRAT, BUFQ_SORT_RAWBLOCK);

    sc->openings = 1; /* wazziz?*/

	aprint_naive("\n");

    /* setup all required fields so that if the attach fails we are ok */
	sc->sc_dk.dk_driver = &eflashdkdriver;
	sc->sc_dk.dk_name = device_xname(sc->sc_dev);

	format_bytes(pbuf, sizeof(pbuf), sc->sc_capacity * DEV_BSIZE);
	aprint_normal("%s: %s, %d cyl, %d head, %d sec, %d bytes/sect x %llu sectors\n",
	    device_xname(self), pbuf, 1, 1, sc->sc_capacity,
	    DEV_BSIZE, (unsigned long long)sc->sc_capacity);

    eflash_set_geometry(sc);

	/*
	 * Attach the disk structure. We fill in dk_info later.
	 */
	disk_attach(&sc->sc_dk);

	rnd_attach_source(&sc->rnd_source, device_xname(sc->sc_dev),
			  RND_TYPE_DISK, RND_FLAG_DEFAULT);

}

int
eflashactivate(device_t self, enum devact act)
{
	int rv = 0;

	DEBUG_PRINT(("eflashactivate %x\n",  act), DEBUG_FUNCS | DEBUG_PROBE);

	switch (act) {
	case DVACT_DEACTIVATE:
		/*
		 * Nothing to do; we key off the device's DVF_ACTIVATE.
		 */
		break;
	default:
		rv = EOPNOTSUPP;
		break;
	}
	return (rv);
}

int
eflashdetach(device_t self, int flags)
{
	struct eflash_softc *sc = device_private(self);
	int s, bmaj, cmaj, i, mn;

	DEBUG_PRINT(("%s: eflashdetach\n",  device_xname(sc->sc_dev)), DEBUG_FUNCS | DEBUG_PROBE);

	/* locate the major number */
	bmaj = bdevsw_lookup_major(&eflash_bdevsw);
	cmaj = cdevsw_lookup_major(&eflash_cdevsw);

	/* Nuke the vnodes for any open instances. */
	for (i = 0; i < MAXPARTITIONS; i++) {
		mn = EFLASHMINOR(device_unit(self), i);
		vdevgone(bmaj, mn, mn, VBLK);
		vdevgone(cmaj, mn, mn, VCHR);
	}

	/* Delete all of our wedges. */
	dkwedge_delall(&sc->sc_dk);

	s = splbio();

	/* Kill off any queued buffers. */
	bufq_drain(sc->sc_q);

	bufq_free(sc->sc_q);
	/*sc->atabus->ata_killpending(sc->drvp);*/

	splx(s);

	/* Detach disk. */
	disk_detach(&sc->sc_dk);

	/* Unhook the entropy source. */
	rnd_detach_source(&sc->rnd_source);

	/*sc->drvp->drive_flags = 0; -- no drive any more here */

	return (0);
}

extern int	dkwedge_autodiscover;

/* Aux temp thread to avoid deadlock when doing the partitio.. ahem wedges thing.
 */
static void
eflash_wedges(void *arg)
{
	struct eflash_softc *sc = (struct eflash_softc*)arg;

    DBGME(DEBUG_STATUS,printf("%s: wedges started for %p\n", sc->sc_dk.dk_name, sc));

	/* Discover wedges on this disk. */
    dkwedge_autodiscover = 1;
	dkwedge_discover(&sc->sc_dk);

    config_pending_decr(sc->sc_dev);

    DBGME(DEBUG_STATUS,printf("%s: wedges thread done for %p\n", device_xname(sc->sc_dev), sc));
	kthread_exit(0);
}

static void
eflash_thread(void *arg)
{
	struct eflash_softc *sc = (struct eflash_softc*)arg;
	struct buf *bp;
    vaddr_t addr;
	int s, error;

    DBGME(DEBUG_STATUS,printf("%s: thread started for %p\n", device_xname(sc->sc_dev), sc));

    s = splbio();
    eflashattach(sc);
    splx(s);

    /* Allocate a VM window large enough to map the largest sector
     * BUGBUG We could risk it and allocate/free on open/close?
     */
    addr = uvm_km_alloc(kernel_map, sc->sc_max_secsize, 0, UVM_KMF_VAONLY);
    if (addr == 0)
        panic("eflash_thread: kernel map full (%lx)", (long unsigned)sc->sc_max_secsize);
    sc->sc_sector = (/*volatile*/ uint8_t *) addr;
    sc->sc_sector_size = 0;
    sc->sc_sector_offset = NOSECTOR;

	error = kthread_create(PRI_NONE, 0, NULL,
	    eflash_wedges, sc, NULL, "%s.wedges", device_xname(sc->sc_dev));
	if (error) {
		aprint_error_dev(sc->sc_dev, "wedges: unable to create kernel "
		    "thread: error %d\n", error);
		/* XXX: why continue? */
	}


    DBGME(DEBUG_STATUS,printf("%s: thread service active for %p\n", device_xname(sc->sc_dev), sc));

    s = splbio();
	for (;;) {
        /* Get next I/O request, wait if necessary
         */
		if ((sc->ch_flags & (ATACH_TH_RESET | ATACH_SHUTDOWN)) == 0 &&
		    (sc->active_xfer == NULL)) {
			sc->ch_flags &= ~ATACH_TH_RUN;
			(void) tsleep(&sc->ch_thread, PRIBIO, "eflashth", 0);
			sc->ch_flags |= ATACH_TH_RUN;
		}
		if (sc->ch_flags & ATACH_SHUTDOWN) {
			break;
        }
        bp = sc->active_xfer;
        sc->active_xfer = NULL;
		if (bp != NULL) {

            size_t sz = DEV_BSIZE, bnow;

            DBGME(DEBUG_XFERS,printf("%s: task %p %x %p %qx %d (%zd)\n", device_xname(sc->sc_dev), bp,
                                     sc->sc_bio.flags, sc->sc_bio.databuf, sc->sc_bio.blkno,
                                     sc->sc_bio.nbytes, sc->sc_bio.nblks));

            sc->sc_bio.error = 0;
            for (; sc->sc_bio.nblks > 0;) {

                bnow = sc->sc_bio.nblks;
                if (sc->sc_bio.flags & ATA_SINGLE) bnow = 1;

                if (sc->sc_bio.flags & ATA_READ) {
                    sc->sc_bio.error = 
                        eflash_read_at(sc, sc->sc_bio.blkno, sc->sc_bio.databuf, bnow, &sz);
                } else {
                    sc->sc_bio.error = 
                        eflash_write_at(sc, sc->sc_bio.blkno, sc->sc_bio.databuf, bnow, &sz);
                }

                if (sc->sc_bio.error)
                    break;

                sc->sc_bio.blkno += sz; /* in blocks */
                sc->sc_bio.nblks -= sz;
                sc->sc_bio.blkdone += sz;
                sz = sz << DEV_BSHIFT; /* in bytes */
                sc->sc_bio.databuf += sz;
                sc->sc_bio.nbytes  -= sz;
            }

            eflashdone(sc);
        }
	}

	splx(s);
	sc->ch_thread = NULL;
	wakeup(&sc->ch_flags);

    DBGME(DEBUG_STATUS,printf("%s: thread service terminated for %p\n", device_xname(sc->sc_dev), sc));

	kthread_exit(0);
}


/*
 * Read/write routine for a buffer.  Validates the arguments and schedules the
 * transfer.  Does not wait for the transfer to complete.
 */
void
eflashstrategy(struct buf *bp)
{
	struct eflash_softc *sc = device_lookup_private(&eflash_cd, EFLASHUNIT(bp->b_dev));
	struct disklabel *lp = sc->sc_dk.dk_label;
	daddr_t blkno;
	int s;

	DEBUG_PRINT(("%s: eflashstrategy %lld\n", device_xname(sc->sc_dev), bp->b_blkno),
	    DEBUG_XFERS);

	/* Valid request?  */
	if (bp->b_blkno < 0 ||
	    (bp->b_bcount % lp->d_secsize) != 0 ||
	    (bp->b_bcount / lp->d_secsize) >= (1 << NBBY)) {
		bp->b_error = EINVAL;
		goto done;
	}

	/* If device invalidated (e.g. media change, door open), error. */
	if ((sc->sc_flags & EFLASHF_LOADED) == 0) {
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
	if (EFLASHPART(bp->b_dev) == RAW_PART) {
		if (bounds_check_with_mediasize(bp, DEV_BSIZE,
		    sc->sc_capacity) <= 0)
			goto done;
	} else {
		if (bounds_check_with_label(&sc->sc_dk, bp,
		    (sc->sc_flags & (EFLASHF_WLABEL|EFLASHF_LABELLING)) != 0) <= 0)
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

	if (EFLASHPART(bp->b_dev) != RAW_PART)
		blkno += lp->d_partitions[EFLASHPART(bp->b_dev)].p_offset;

	bp->b_rawblkno = blkno;

	/* Queue transfer on drive, activate drive and controller if idle. */
	s = splbio();
	bufq_put(sc->sc_q, bp);
	eflashstart(sc);
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
eflashstart(void *arg)
{
	struct eflash_softc *sc = arg;
	struct buf *bp = NULL;

	DEBUG_PRINT(("%s: eflashstart\n", device_xname(sc->sc_dev)),
	    DEBUG_XFERS);
	while (sc->openings > 0) {

		/* Is there a buf for us ? */
		if ((bp = bufq_get(sc->sc_q)) == NULL)
			return;

		/*
		 * Make the command. First lock the device
		 */
		sc->openings--;

		sc->retries = 0;
		__eflashstart(sc, bp);
	}
}

void
__eflashstart(struct eflash_softc *sc, struct buf *bp)
{
	DEBUG_PRINT(("%s: __eflashstart %p\n", device_xname(sc->sc_dev), bp),
	    DEBUG_XFERS);

	sc->sc_bp = bp;
	/*
	 * If we're retrying, retry in single-sector mode. This will give us
	 * the sector number of the problem, and will eventually allow the
	 * transfer to succeed.
	 */
	if (sc->retries >= EFLASHIORETRIES_SINGLE)
		sc->sc_bio.flags = ATA_SINGLE;
	else
		sc->sc_bio.flags = 0;
	if (bp->b_flags & B_READ)
		sc->sc_bio.flags |= ATA_READ;
	sc->sc_bio.blkno = bp->b_rawblkno;
	sc->sc_bio.blkdone = 0;
	sc->sc_bio.nbytes = bp->b_bcount;
	sc->sc_bio.nblks  = bp->b_bcount >> DEV_BSHIFT;
	sc->sc_bio.databuf = bp->b_data;
	/* Instrumentation. */
	disk_busy(&sc->sc_dk);
    sc->active_xfer = bp;
    wakeup(&sc->ch_thread);
}

void
eflashdone(struct eflash_softc *sc)
{
	struct buf *bp = sc->sc_bp;
	const char *errmsg;
	int do_perror = 0;

	DEBUG_PRINT(("%s: eflashdone %p\n", device_xname(sc->sc_dev), bp),
	    DEBUG_XFERS);

	if (bp == NULL)
		return;

	bp->b_resid = sc->sc_bio.nbytes;
	switch (sc->sc_bio.error) {
	case ETIMEDOUT:
		errmsg = "device timeout";
        do_perror = 1;
		goto retry;
	case EBUSY:
		errmsg = "device stuck";
retry:		/* Just reset and retry. Can we do more ? */
		/*eflash_reset(sc);*/
		diskerr(bp, "flash", errmsg, LOG_PRINTF,
		    sc->sc_bio.blkdone, sc->sc_dk.dk_label);
		if (sc->retries < EFLASHIORETRIES)
			printf(", retrying");
		printf("\n");
		if (do_perror)
			eflashperror(sc);
		if (sc->retries < EFLASHIORETRIES) {
			sc->retries++;
			callout_reset(&sc->sc_restart_ch, RECOVERYTIME,
			    eflashrestart, sc);
			return;
		}

		bp->b_error = EIO;
		break;
	case 0:
        if ((sc->sc_bio.flags & ATA_CORR) || sc->retries > 0)
			printf("%s: soft error (corrected)\n",
			    device_xname(sc->sc_dev));
		break;
	case ENODEV:
	case E2BIG:
		bp->b_error = EIO;
		break;
	}
	disk_unbusy(&sc->sc_dk, (bp->b_bcount - bp->b_resid),
	    (bp->b_flags & B_READ));
	rnd_add_uint32(&sc->rnd_source, bp->b_blkno);
    biodone(bp);
    sc->openings++;
	eflashstart(sc);
}

void
eflashrestart(void *v)
{
	struct eflash_softc *sc = v;
	struct buf *bp = sc->sc_bp;
	int s;
	DEBUG_PRINT(("%s: eflashrestart\n", device_xname(sc->sc_dev)),
	    DEBUG_XFERS);

	s = splbio();
	__eflashstart(v, bp);
	splx(s);
}

int
eflashread(dev_t dev, struct uio *uio, int flags)
{
	DEBUG_PRINT(("eflashread\n"), DEBUG_XFERS);
	return (physio(eflashstrategy, NULL, dev, B_READ, minphys, uio));
}

int
eflashwrite(dev_t dev, struct uio *uio, int flags)
{
	DEBUG_PRINT(("eflashwrite\n"), DEBUG_XFERS);
	return (physio(eflashstrategy, NULL, dev, B_WRITE, minphys, uio));
}

int
eflashopen(dev_t dev, int flag, int fmt, struct lwp *l)
{
	struct eflash_softc *sc;
	int part, error;

	DEBUG_PRINT(("eflashopen %" PRIx64 "\n", dev), DEBUG_FUNCS);
	sc = device_lookup_private(&eflash_cd, EFLASHUNIT(dev));
	if (sc == NULL)
		return (ENXIO);

	if (! device_is_active(sc->sc_dev))
		return (ENODEV);

	part = EFLASHPART(dev);

	mutex_enter(&sc->sc_dk.dk_openlock);

	/*
	 * If there are wedges, and this is not RAW_PART, then we
	 * need to fail.
	 */
	if (sc->sc_dk.dk_nwedges != 0 && part != RAW_PART) {
		error = EBUSY;
		goto bad;
	}

	if (sc->sc_dk.dk_openmask != 0) {
		/*
		 * If any partition is open, but the disk has been invalidated,
		 * disallow further opens.
		 */
		if ((sc->sc_flags & EFLASHF_LOADED) == 0) {
			error = EIO;
			goto bad;
		}
	} else {
		if ((sc->sc_flags & EFLASHF_LOADED) == 0) {
			sc->sc_flags |= EFLASHF_LOADED;

			/* Load the partition info if not already loaded. */
			eflashgetdisklabel(sc);
		}
	}

	/* Check that the partition exists. */
	if (part != RAW_PART &&
	    (part >= sc->sc_dk.dk_label->d_npartitions ||
	     sc->sc_dk.dk_label->d_partitions[part].p_fstype == FS_UNUSED)) {
		error = ENXIO;
		goto bad;
	}

	/* Insure only one open at a time. */
	switch (fmt) {
	case S_IFCHR:
		sc->sc_dk.dk_copenmask |= (1 << part);
		break;
	case S_IFBLK:
		sc->sc_dk.dk_bopenmask |= (1 << part);
		break;
	}
	sc->sc_dk.dk_openmask =
	    sc->sc_dk.dk_copenmask | sc->sc_dk.dk_bopenmask;

	mutex_exit(&sc->sc_dk.dk_openlock);
	return 0;

 bad:
	mutex_exit(&sc->sc_dk.dk_openlock);
	DEBUG_PRINT(("%s: eflashopen -> %d\n", device_xname(sc->sc_dev), error),
	    DEBUG_XFERS);
	return error;
}

int
eflashclose(dev_t dev, int flag, int fmt, struct lwp *l)
{
	struct eflash_softc *sc = device_lookup_private(&eflash_cd, EFLASHUNIT(dev));
	int part = EFLASHPART(dev);

	DEBUG_PRINT(("eflashclose %" PRIx64 "\n", dev), DEBUG_FUNCS);

	mutex_enter(&sc->sc_dk.dk_openlock);

	switch (fmt) {
	case S_IFCHR:
		sc->sc_dk.dk_copenmask &= ~(1 << part);
		break;
	case S_IFBLK:
		sc->sc_dk.dk_bopenmask &= ~(1 << part);
		break;
	}
	sc->sc_dk.dk_openmask =
	    sc->sc_dk.dk_copenmask | sc->sc_dk.dk_bopenmask;

	if (sc->sc_dk.dk_openmask == 0) {

		if (! (sc->sc_flags & EFLASHF_KLABEL))
			sc->sc_flags &= ~EFLASHF_LOADED;

        DEBUG_PRINT(("%s: eflashclose flg %x\n", device_xname(sc->sc_dev), sc->sc_flags),
                    DEBUG_XFERS);

	}

	mutex_exit(&sc->sc_dk.dk_openlock);
	return 0;
}

void
eflashgetdefaultlabel(struct eflash_softc *sc, struct disklabel *lp)
{

	DEBUG_PRINT(("%s: eflashgetdefaultlabel\n", device_xname(sc->sc_dev)), DEBUG_FUNCS);
	memset(lp, 0, sizeof(struct disklabel));

	lp->d_secsize = DEV_BSIZE;
	lp->d_ntracks = 1;
	lp->d_nsectors = sc->sc_capacity;
	lp->d_ncylinders = 1;
	lp->d_secpercyl = lp->d_ntracks * lp->d_nsectors;

    lp->d_type = DTYPE_ST506; /* ?!? */

	strncpy(lp->d_typename, ST506, 16);
	strncpy(lp->d_packname, "fictitious", 16);
	if (sc->sc_capacity > UINT32_MAX)
		lp->d_secperunit = UINT32_MAX;
	else
		lp->d_secperunit = sc->sc_capacity;
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
eflashgetdisklabel(struct eflash_softc *sc)
{
	struct disklabel *lp = sc->sc_dk.dk_label;
	const char *errstring;

	DEBUG_PRINT(("%s: eflashgetdisklabel\n",  device_xname(sc->sc_dev)), DEBUG_FUNCS);

	memset(sc->sc_dk.dk_cpulabel, 0, sizeof(struct cpu_disklabel));

	eflashgetdefaultlabel(sc, lp);

#ifdef HAS_BAD144_HANDLING
	sc->sc_bio.badsect[0] = -1;
#endif

    /* BUGBUG: maj==0?? why is this not EFLASHLABELDEV(??sc->sc_dev) */
	errstring = readdisklabel(MAKEEFLASHDEV(0, device_unit(sc->sc_dev),
				  RAW_PART), eflashstrategy, lp,
				  sc->sc_dk.dk_cpulabel);
	if (errstring) {
		printf("%s: %s\n", device_xname(sc->sc_dev), errstring);
		return;
	}

#if DEBUG
    if (EFLASH_DEBUG(DEBUG_WRITES)) {
        int i, n = sc->sc_dk.dk_label->d_npartitions;
        printf("%s: %d parts\n", device_xname(sc->sc_dev), n);
        for (i = 0; i < n; i++) {
            printf("\t[%d]: t=%x s=%d o=%d\n", i,
                   sc->sc_dk.dk_label->d_partitions[i].p_fstype,
                   sc->sc_dk.dk_label->d_partitions[i].p_size,
                   sc->sc_dk.dk_label->d_partitions[i].p_offset);
        }
    }
#endif

#ifdef HAS_BAD144_HANDLING
	if ((lp->d_flags & D_BADSECT) != 0)
		bad144intern(sc);
#endif
}

void
eflashperror(const struct eflash_softc *sc)
{
	const char *devname = device_xname(sc->sc_dev);
	u_int32_t Status = sc->sc_bio.r_error;

	printf("%s: (", devname);

	if (Status == 0)
		printf("error not notified");
    else
        printf("status=x%x", Status);

	printf(")\n");
}

int
eflashioctl(dev_t dev, u_long xfer, void *addr, int flag, struct lwp *l)
{
	struct eflash_softc *sc = device_lookup_private(&eflash_cd, EFLASHUNIT(dev));
	int error = 0, s;

	DEBUG_PRINT(("eflashioctl(%lx)\n",xfer), DEBUG_FUNCS);

	if ((sc->sc_flags & EFLASHF_LOADED) == 0)
		return EIO;

	error = disk_ioctl(&sc->sc_dk, xfer, addr, flag, l);
	if (error != EPASSTHROUGH)
		return (error);

	switch (xfer) {
#ifdef HAS_BAD144_HANDLING
	case DIOCSBAD:
		if ((flag & FWRITE) == 0)
			return EBADF;
		sc->sc_dk.dk_cpulabel->bad = *(struct dkbad *)addr;
		sc->sc_dk.dk_label->d_flags |= D_BADSECT;
		bad144intern(sc);
		return 0;
#endif
	case DIOCGDINFO:
		*(struct disklabel *)addr = *(sc->sc_dk.dk_label);
		return 0;

	case DIOCGPART:
		((struct partinfo *)addr)->disklab = sc->sc_dk.dk_label;
		((struct partinfo *)addr)->part =
		    &sc->sc_dk.dk_label->d_partitions[EFLASHPART(dev)];
		return 0;

	case DIOCWDINFO:
	case DIOCSDINFO:
	{
		struct disklabel *lp;

		if ((flag & FWRITE) == 0)
			return EBADF;

		lp = (struct disklabel *)addr;

		mutex_enter(&sc->sc_dk.dk_openlock);
		sc->sc_flags |= EFLASHF_LABELLING;

		error = setdisklabel(sc->sc_dk.dk_label,
		    lp, /*sc->sc_dk.dk_openmask : */0,
		    sc->sc_dk.dk_cpulabel);
		if (error == 0) {
			if (xfer == DIOCWDINFO)
				error = writedisklabel(EFLASHLABELDEV(dev),
				    eflashstrategy, sc->sc_dk.dk_label,
				    sc->sc_dk.dk_cpulabel);
		}

		sc->sc_flags &= ~EFLASHF_LABELLING;
		mutex_exit(&sc->sc_dk.dk_openlock);
		return error;
	}

	case DIOCKLABEL:
		if (*(int *)addr)
			sc->sc_flags |= EFLASHF_KLABEL;
		else
			sc->sc_flags &= ~EFLASHF_KLABEL;
		return 0;

	case DIOCWLABEL:
		if ((flag & FWRITE) == 0)
			return EBADF;
		if (*(int *)addr)
			sc->sc_flags |= EFLASHF_WLABEL;
		else
			sc->sc_flags &= ~EFLASHF_WLABEL;
		return 0;

	case DIOCGDEFLABEL:
		eflashgetdefaultlabel(sc, (struct disklabel *)addr);
		return 0;

	case DIOCCACHESYNC:
		return 0;

	case DIOCAWEDGE:
	    {
	    	struct dkwedge_info *dkw = (void *) addr;

		if ((flag & FWRITE) == 0)
			return (EBADF);

		/* If the ioctl happens here, the parent is us. */
		strcpy(dkw->dkw_parent, device_xname(sc->sc_dev));
		return (dkwedge_add(dkw));
	    }

	case DIOCDWEDGE:
	    {
	    	struct dkwedge_info *dkw = (void *) addr;

		if ((flag & FWRITE) == 0)
			return (EBADF);

		/* If the ioctl happens here, the parent is us. */
		strcpy(dkw->dkw_parent, device_xname(sc->sc_dev));
		return (dkwedge_del(dkw));
	    }

	case DIOCLWEDGES:
	    {
	    	struct dkwedge_list *dkwl = (void *) addr;

		return (dkwedge_list(&sc->sc_dk, dkwl, l));
	    }

	case DIOCGSTRATEGY:
	    {
		struct disk_strategy *dks = (void *)addr;

		s = splbio();
		strlcpy(dks->dks_name, bufq_getstrategyname(sc->sc_q),
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
		old = sc->sc_q;
		bufq_move(new, old);
		sc->sc_q = new;
		splx(s);
		bufq_free(old);

		return 0;
	    }

	default:
        /* NB: we get a DIOCGWEDGEINFO, but nobody else handles it either */
        DEBUG_PRINT(("eflashioctl: unsup x%lx\n", xfer), DEBUG_FUNCS);
		return ENOTTY;
	}
}

int
eflashsize(dev_t dev)
{
	struct eflash_softc *sc;
	int part, omask;
	int size;

	DEBUG_PRINT(("eflashsize\n"), DEBUG_FUNCS);

	sc = device_lookup_private(&eflash_cd, EFLASHUNIT(dev));
	if (sc == NULL)
		return (-1);

	part = EFLASHPART(dev);
	omask = sc->sc_dk.dk_openmask & (1 << part);

	if (omask == 0 && eflashopen(dev, 0, S_IFBLK, NULL) != 0)
		return (-1);
	if (sc->sc_dk.dk_label->d_partitions[part].p_fstype != FS_SWAP)
		size = -1;
	else
		size = sc->sc_dk.dk_label->d_partitions[part].p_size *
		    (sc->sc_dk.dk_label->d_secsize / DEV_BSIZE);
	if (omask == 0 && eflashclose(dev, 0, S_IFBLK, NULL) != 0)
		return (-1);
	return (size);
}

/*
 * Dump core after a system crash.
 */
int
eflashdump(dev_t dev, daddr_t blkno, void *va, size_t size)
{
    /* no we dont */
    return (ENXIO);
}

#ifdef HAS_BAD144_HANDLING
/*
 * Internalize the bad sector table.
 */
void
bad144intern(struct eflash_softc *sc)
{
	struct dkbad *bt = &sc->sc_dk.dk_cpulabel->bad;
	struct disklabel *lp = sc->sc_dk.dk_label;
	int i = 0;

	DEBUG_PRINT(("bad144intern\n"), DEBUG_XFERS);

	for (; i < NBT_BAD; i++) {
		if (bt->bt_bad[i].bt_cyl == 0xffff)
			break;
		sc->sc_bio.badsect[i] =
		    bt->bt_bad[i].bt_cyl * lp->d_secpercyl +
		    (bt->bt_bad[i].bt_trksec >> 8) * lp->d_nsectors +
		    (bt->bt_bad[i].bt_trksec & 0xff);
	}
	for (; i < NBT_BAD+1; i++)
		sc->sc_bio.badsect[i] = -1;
}
#endif

static void
eflash_set_geometry(struct eflash_softc *sc)
{
	struct disk_geom *dg = &sc->sc_dk.dk_geom;

	memset(dg, 0, sizeof(*dg));

	dg->dg_secperunit = sc->sc_capacity;
	dg->dg_secsize = DEV_BSIZE /* XXX 512? */;
	dg->dg_nsectors = sc->sc_capacity;
	dg->dg_ntracks = 1;
	dg->dg_ncylinders = sc->sc_capacity;

	disk_set_info(sc->sc_dev, &sc->sc_dk, ST506);
}
