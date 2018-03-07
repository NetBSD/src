/*	$NetBSD: ace.c,v 1.4 2018/03/07 14:59:14 christos Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
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

/* --------------------------------------------------------------------------
 *
 * Module:
 *
 *    ace.c
 *
 * Purpose:
 *
 *    Driver for the Xilinx System ACE CompactFlash Solution
 *
 * Author:
 *    A. Forin (sandrof)
 *
 * References:
 *    "System ACE CompactFlash Solution", Advance Product Specification
 *    Document DS080 Version 1.5 April 5, 2002.  Xilinx Corp.
 *    Available at http://www.xilinx.com
 *
 *    "CF+ and CompactFlash Specification", Revision 4.1, 02/16/2007.
 *    CompactFlash Association.
 *    Available at http://www.compactflash.org
 * --------------------------------------------------------------------------
 */

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>
#include <machine/emipsreg.h>

#include <sys/param.h>
#include <sys/disklabel.h>
#include <sys/endian.h>

#include "common.h"
#include "ace.h"
#include "start.h"

#define NSAC 2
#define SAC0 ((struct _Sac  *)IDE_DEFAULT_ADDRESS)
#define SAC1 ((struct _Sac  *)(IDE_DEFAULT_ADDRESS+256))

#define CF_SECBITS     9
#define CF_SECTOR_SIZE (1 << CF_SECBITS)


/* Error codes 
 */
#define FAILED(x)           (x < 0)
#define S_OK                (0)
#define E_INVALID_PARAMETER (-1)
#define E_DISK_RESET_FAILED (-2)
#define E_NO_MEDIA_IN_DRIVE (-3)
#define E_TIMED_OUT         (-4)

/* Utilities
 */
#if defined(DEBUG)
int acedebug = 2;
#define DBGME(lev,x) \
	do \
		if (lev >= acedebug) { \
			x; \
		} \
	while (/*CONSTCOND*/0)
#else
#define DBGME(lev,x) 
#endif

#if defined(DEBUG)
typedef char *NAME;
typedef struct _REGDESC {
    NAME RegisterName;
    NAME BitNames[32];
} REGDESC, *PREGDESC;

static void SysacePrintRegister(const REGDESC *Desc, uint32_t Value);

static void SysacePrintRegister(const REGDESC *Desc, uint32_t Value)
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

static void SysaceDumpRegisters(struct _Sac *Interface)
{
	const REGDESC Control_Names =
        { "Control",
          {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
          "RST", //			0x00010000
          "BUS8", //		0x00020000
          "BUS16", //		0x00040000
          "BUS32", //		0x00080000
          "IRQ", //			0x00100000
          "BRDY", //		0x00200000
          "IMSK0", //		0x00400000
          "IMSK1", //		0x00800000
          "TD0", //	    	0x0f000000
          "TD1", //		    0x0f000000
          "TD2", //	    	0x0f000000
          "TD3", //	    	0x0f000000
          "BUFW8", //		0x10000000
          "BUFW16", //		0x20000000
          "BUFW32", //		0x40000000
          "DEBUG"} //		0x80000000
        };

	const REGDESC STATUS_Names =
        { "STATUS",
          {"CFGLOCK", //	0x00000001
          "MPULOCK", //		0x00000002
          "CFGERROR", //	0x00000004
          "CFCERROR", //	0x00000008
          "CFDETECT", //	0x00000010
          "DATABUFRDY", //	0x00000020
          "DATABUFWRITE", //0x00000040
          "CFGDONE", //		0x00000080
          "RDYFORCFCMD", //	0x00000100
          "CFGMODEPIN", //	0x00000200
          0,0,0,
          "CFGADDRPIN0", //	0x0000e000
          "CFGADDRPIN1", //	0x0000e000
          "CFGADDRPIN2", //	0x0000e000
          0,
          "CFBSY", //		0x00020000
          "CFRDY", //		0x00040000
          "CFDWF", //		0x00080000
          "CFDSC", //		0x00100000
          "CFDRQ", //		0x00200000
          "CFCORR", //		0x00400000
          "CFERR", //		0x00800000
          0,}
        };

	const REGDESC ERRORREG_Names =
        { "ERRORREG",
          {"CARDRESETERR", //			0x00000001
          "CARDRDYERR", //				0x00000002
          "CARDREADERR", //				0x00000004
          "CARDWRITEERR", //			0x00000008
          "SECTORRDYERR", //			0x00000010
          "CFGADDRERR", //				0x00000020
          "CFGFAILED", //				0x00000040
          "CFGREADERR", //				0x00000080
          "CFGINSTRERR", //				0x00000100
          "CFGINITERR", //				0x00000200
          0,
          "CFBBK", //					0x00000800
          "CFUNC", //					0x00001000
          "CFIDNF", //					0x00002000
          "CFABORT", //					0x00004000
          "CFAMNF", //					0x00008000
           0,}
        };

    const NAME CommandNames[8] =
        { "0", //     				0x0000
          "RESETMEMCARD", //   		0x0100
          "IDENTIFYMEMCARD", //		0x0200
          "READMEMCARDDATA", //		0x0300
          "WRITEMEMCARDDATA", //  	0x0400
          "5", //     				0x0500
          "ABORT", //			    0x0600
          "7" //     			    0x0700
        };

	const REGDESC CONTROLREG_Names =
        { "CONTROLREG",
          {"FORCELOCKREQ", //			0x00000001
          "LOCKREQ", //					0x00000002
          "FORCECFGADDR", //			0x00000004
          "FORCECFGMODE", //			0x00000008
          "CFGMODE", //					0x00000010
          "CFGSTART", //				0x00000020
          "CFGSEL_MPU", //  			0x00000040
          "CFGRESET", //				0x00000080
          "DATABUFRDYIRQ", //			0x00000100
          "ERRORIRQ", //				0x00000200
          "CFGDONEIRQ", //				0x00000400
          "RESETIRQ", //				0x00000800
          "CFGPROG", //					0x00001000
          "CFGADDR_B0", //				0x00002000
          "CFGADDR_B1", //				0x00004000
          "CFGADDR_B2", //				0x00008000
          0,}
        };

	const REGDESC FATSTATREG_Names =
        { "FATSTATREG",
          {"MBRVALID", //				0x00000001
          "PBRVALID", //				0x00000002
          "MBRFAT12", //				0x00000004
          "PBRFAT12", //				0x00000008
          "MBRFAT16", //				0x00000010
          "PBRFAT16", //				0x00000020
          "CALCFAT12", //				0x00000040
          "CALCFAT16", //				0x00000080
          0, }
        };

    printf("Sysace@%p:\n", Interface);
    printf("\tTag %x\n", Interface->Tag);
    SysacePrintRegister(&Control_Names, Interface->Control);
    printf("\tBUSMODEREG %x\n", Interface->BUSMODEREG);
    SysacePrintRegister(&STATUS_Names, Interface->STATUS);
    SysacePrintRegister(&ERRORREG_Names, Interface->ERRORREG);
    printf("\tCFGLBAREG %x\n", Interface->CFGLBAREG);
    printf("\tMPULBAREG %x\n", Interface->MPULBAREG);
    printf("\tVERSIONREG %x\n", Interface->VERSIONREG);
    printf("\tSECCNTCMDREG %x = %s cnt=%d\n", Interface->SECCNTCMDREG,
             CommandNames[(Interface->SECCNTCMDREG >> 8)&7],
             Interface->SECCNTCMDREG & SAC_SECCCNT);
    SysacePrintRegister(&CONTROLREG_Names, Interface->CONTROLREG);
    SysacePrintRegister(&FATSTATREG_Names, Interface->FATSTATREG);
}

#else
#define SysaceDumpRegisters(_c_)
#endif

/* Reset the device and the interface
 */
static int SysaceInitialize(struct _Sac *Interface)
{
    /* 16bit mode etc etc */
	uint32_t BusMode, Control;

    /* reset our interface */
    Interface->Control = SAC_RST;
    Delay(200);

    /* repeat on both byte lanes */
	Interface->BUSMODEREG = SAC_MODE16 | (SAC_MODE16 << 8);
    Delay(1);

    /* check what our interface does and what the SysACE expects */
	Control = Interface->Control;
	BusMode = Interface->BUSMODEREG;

    /* get them to agree */
	if (BusMode & SAC_MODE16)
	{
		Interface->Control = Control | SAC_BUS16;
		Interface->Control = Interface->Control & ~SAC_BUS8;
	}
	else
	{
		Interface->Control = Control | SAC_BUS8;
		Interface->Control = Interface->Control & ~SAC_BUS16;
	}

    /* check that it worked */
	BusMode = Interface->BUSMODEREG;
	Control = Interface->Control;

	if (((BusMode & SAC_MODE16) == 0) && ((Control & SAC_BUS8) == 0)) return E_DISK_RESET_FAILED;
	if (((BusMode & SAC_MODE16) > 0) && ((Control & SAC_BUS16) == 0)) return E_DISK_RESET_FAILED;

    /* interrupts off for now */
    Interface->Control &= ~SAC_INTMASK;
#define SAC_INTERRUPTS (SAC_DATABUFRDYIRQ |	SAC_ERRORIRQ )// | SAC_CFGDONEIRQ)
    Control = Interface->CONTROLREG;
    Control = (Control & ~SAC_INTERRUPTS) | SAC_RESETIRQ | SAC_FORCECFGMODE;
    Interface->CONTROLREG = Control;
    Interface->CONTROLREG = Control & ~SAC_RESETIRQ;

    /* no command */
    Interface->MPULBAREG = 0;

    return S_OK;
}

/* Take control of the ACE datapath
 */
static int SysaceLock(struct _Sac *Interface)
{
    uint32_t Status;
    int i;

    /* Locked already?
     */
    Status = Interface->STATUS;
    if (Status & SAC_MPULOCK)
        return TRUE;

    /* Request lock
     */
    Interface->CONTROLREG |= SAC_LOCKREQ;

    /* Spin a bit until we get it
     */
    for (i = 0; i < 200; i++) {
        Status = Interface->STATUS;
        if (Status & SAC_MPULOCK)
            return TRUE;
        Delay(100);
        DBGME(0,printf("Sysace::Lock loops.. (st=%x)\n",Status));
    }

    /* oopsie!
     */
    DBGME(3,printf("Sysace::Lock timeout (st=%x)\n",Status));
    SysaceDumpRegisters(Interface);
    return FALSE;
}

/* Release control of the ACE datapath
 */
static int SysaceUnlock(struct _Sac *Interface)
{
    uint32_t Status;
    int i;

    /* Clear reset 
     */
    Interface->CONTROLREG &= ~SAC_CFGRESET;

    /* Unlocked already?
     */
    Status = Interface->STATUS;
    if (0 == (Status & SAC_MPULOCK))
        return TRUE;

    /* Request unlock
     */
    Interface->CONTROLREG &= ~SAC_LOCKREQ;

    /* Spin a bit until we get it
     */
    for (i = 0; i < 200; i++) {
        Status = Interface->STATUS;
        if (0 == (Status & SAC_MPULOCK))
            return TRUE;
        Delay(100);
        DBGME(0,printf("Sysace::Unlock loops.. (st=%x)\n",Status));
    }

    /* oopsie!
     */
    DBGME(3,printf("Sysace::Unlock timeout (st=%x)\n",Status));
    SysaceDumpRegisters(Interface);
    return FALSE;
}

/* Check if the ACE is waiting for a comamnd
 */
#define SysaceReadyForCommand(_i_) ((_i_)->STATUS & SAC_RDYFORCFCMD)

/* Check if the ACE is executing a comamnd
 */
#define SysaceBusyWithCommand(_i_) ((_i_)->STATUS & SAC_CFBSY)

/* Turn on interrupts from the ACE
 */
#define SysaceInton(_i_) { \
          (_i_)->CONTROLREG |= SAC_INTERRUPTS; \
          (_i_)->Control |= SAC_INTMASK; \
}

/* Turn off interrupts from the ACE
 */
#define SysaceIntoff(_i_) { \
          (_i_)->CONTROLREG &= ~SAC_INTERRUPTS; \
          (_i_)->Control &= ~SAC_INTMASK; \
}

/* Start a command on the ACE, such as read or identify.
 */
static int SysaceStartCommand(struct _Sac *Interface,
                              uint32_t Command,
                              uint32_t Lba,
                              uint32_t nSectors)
{
    int sc = E_DISK_RESET_FAILED;

    /* Lock it if not already
     */
    if (!SysaceLock(Interface)) {
        /* printed already */
        return sc;
    }

    /* Is there a CF inserted
     */
    if (! (Interface->STATUS & SAC_CFDETECT)) {
        /* NB: Not a failure state */
        DBGME(2,printf("Sysace:: no media (st=%x)\n",Interface->STATUS));
        return E_NO_MEDIA_IN_DRIVE;
    }

    /* Is it ready for a command
     */
    if (!SysaceReadyForCommand(Interface)) {
        DBGME(3,printf("Sysace:: not ready (st=%x)\n",Interface->STATUS));
        SysaceDumpRegisters(Interface);
        return sc;
    }

    /* sector number and command
     */
    Interface->MPULBAREG = Lba;
    Interface->SECCNTCMDREG = (uint16_t)(Command | (nSectors & SAC_SECCCNT));

    /* re-route the chip
     * NB: The word "RESET" is actually not much of a misnomer.
     * The chip was designed for a one-shot execution only, at reset time,
     * namely loading the configuration data into the FPGA. So..
     */
    Interface->CONTROLREG |= SAC_CFGRESET;
    return S_OK;
}


/* "Interrupt service routine"
 */
static void SysAce_isr ( struct _Sac *Interface )
{
    uint32_t Control;

    /* Turn off interrupts and ACK them
     */
    SysaceIntoff(Interface);

    Control = Interface->CONTROLREG & (~(SAC_RESETIRQ|SAC_INTERRUPTS));
    Interface->CONTROLREG = Control | SAC_RESETIRQ;
    Interface->CONTROLREG = Control;
}

static int SysAce_wait(struct _Sac *Interface)
{
    int i;
    for (i = 0; i < 30000; i++) {
        if (Interface->STATUS & SAC_DATABUFRDY) {
            SysAce_isr(Interface);
            return S_OK;
        }
        Delay(100);
    }
    return E_TIMED_OUT;
}


/* Read NBLOCKS blocks of 512 bytes each,
 * starting at block number STARTSECTOR,
 * into the buffer BUFFER.
 * Return 0 if ok, -1 otherwise.
 */
static int SysAce_read(struct _Sac * Interface, char *Buffer, uint32_t StartSector, uint32_t nBlocks)
{
    int sc = S_OK;
    uint32_t Size, SizeThisTime;
    uint32_t Status = 0, SizeRead = 0;
    unsigned int i, j;

    Size = nBlocks << CF_SECBITS;

    DBGME(1,printf("Sysace Read(%p %x %x)\n",
                     Buffer, StartSector, nBlocks));

    /* Repeat until we are done or error
     */
    while (sc == S_OK) {

        /* .. one sector at a time
         * BUGBUG Supposedly we can do up to 256 sectors?
         */
        SizeThisTime = Size;
        if (SizeThisTime > CF_SECTOR_SIZE)
            SizeThisTime = CF_SECTOR_SIZE;

        /*  Start a new sector read
         */
        SysaceInton(Interface);
        sc = SysaceStartCommand(Interface,
                                SAC_CMD_READMEMCARDDATA,
                                StartSector,
                                1);
        /* And wait until done, if ok
         */
        if (!FAILED(sc)) {
            sc = SysAce_wait(Interface);
        }

        /* Are we doing ok
         */
        if (!FAILED(sc)) {

            /* Get the data out of the ACE
             */
            for (i = 0; i < SizeThisTime; i += 4) {

                /* Make sure the FIFO is ready
                 */
                for (j = 0; j < 100; j++) {
                    Status = Interface->STATUS;
                    if (Status & SAC_DATABUFRDY)
                        break;
                    Delay(10);
                }

                /* Got it?
                 */
                if (Status & SAC_DATABUFRDY) {
                    uint32_t Data32;
                    
                    Data32 = Interface->DATABUFREG[0];
                    Data32 = le32toh(Data32);
		    DBGME(0,printf(" %x", Data32));
		    if (0 == (0xf & (i+4))) DBGME(0,printf("\n"));
                    memcpy(Buffer+i, &Data32, 4);
                }
                else
                {
                    /* Ooops, get out of here
                     */
                    DBGME(3,printf("Sysace::READ timeout\n"));
                    SysaceDumpRegisters(Interface);
                    sc = E_TIMED_OUT;
                    break;
                }
            }

            /* Still doing ok?
             */
            if (!FAILED(sc)) {
                StartSector        += 1;
                Buffer             += SizeThisTime;
                SizeRead           += SizeThisTime;
                Size               -= SizeThisTime;
            }
        }

        /* Free the ACE for the JTAG, just in case */
        SysaceUnlock(Interface);

        /* Are we done yet?
         */
        if (Size == 0)
            break;
    }

    return sc;
}

/* Exported interface
 */
struct	ace_softc {
    struct _Sac *sc_dp;         /* I/O regs */
	int	sc_part;		        /* disk partition number */
	struct	disklabel sc_label;	/* disk label for this disk */
};

#define	RF_PROTECTED_SECTORS	64	/* XXX refer to <.../rf_optnames.h> */

int aceprobe(int unit)
{
    struct _Sac *Sac;

    if (unit == 0)
        Sac = SAC0;
    else if (unit == 1)
        Sac = SAC1;
    else 
        return E_INVALID_PARAMETER;

    /* Check the tag to see if its there
     */
    if ((Sac->Tag & SAC_TAG) != PMTTAG_SYSTEM_ACE) {
        DBGME(3,printf("init_ace: bad tag (%x != %x) @x%p\n",
                       Sac->Tag, PMTTAG_SYSTEM_ACE, Sac));
        return E_INVALID_PARAMETER;
    }

    return S_OK;
}

/* aceopen("", ctlr, unit, part);
 */
int
aceopen(struct open_file *f, ...)
{
	int ctlr, unit, part;
    struct _Sac *Sac;

	struct ace_softc *sc;
	struct disklabel *lp;
	int i;
	char *msg;
	char buf[DEV_BSIZE];
	size_t cnt;
	va_list ap;

	va_start(ap, f);

	ctlr = va_arg(ap, int);
	unit = va_arg(ap, int);
	part = va_arg(ap, int);
	va_end(ap);

	if (ctlr != 0 || unit >= NSAC || part >= 8)
		return (ENXIO);

    /* Is it there, does it work.
     */
    Sac = (unit == 0) ? SAC0 : SAC1;
    i = aceprobe(unit);
	if (i < 0)
        goto Bad;

    if (SysaceInitialize(Sac) < 0) {
        DBGME(3,printf("ace%d: no reset @x%p\n", unit, Sac));
    Bad:
		printf("open failed\n");
		return (ENXIO);
    }

    /* Yep, go ahead.
     */
	sc = alloc(sizeof(struct ace_softc));
	memset(sc, 0, sizeof(struct ace_softc));
	f->f_devdata = (void *)sc;

	sc->sc_dp = Sac;
	sc->sc_part = part;

	/* try to read disk label and partition table information */
	lp = &sc->sc_label;
	lp->d_secsize = DEV_BSIZE;
	lp->d_secpercyl = 1;
	lp->d_npartitions = MAXPARTITIONS;
	lp->d_partitions[part].p_offset = 0;
	lp->d_partitions[part].p_size = 0x7fffffff;

	i = acestrategy(sc, F_READ, (daddr_t)LABELSECTOR, DEV_BSIZE, buf, &cnt);
	if (i || cnt != DEV_BSIZE) {
		DBGME(3,printf("ace%d: error reading disk label\n", unit));
		goto bad;
	}
	msg = getdisklabel(buf, lp);
	if (msg) {
		/* If no label, just assume 0 and return */
		return (0);
	}

	if (part >= lp->d_npartitions || lp->d_partitions[part].p_size == 0) {
	bad:
		dealloc(sc, sizeof(struct ace_softc));
		DBGME(3,printf("ace%d: bad part %d\n", unit, part));
		return (ENXIO);
	}
	return (0);
}

#ifndef LIBSA_NO_DEV_CLOSE
int
aceclose(struct open_file *f)
{
	dealloc(f->f_devdata, sizeof(struct ace_softc));
	f->f_devdata = (void *)0;
	return (0);
}
#endif

int
acestrategy(
	void *devdata,
	int rw,
	daddr_t bn,
	size_t reqcnt,
	void *addr,
	size_t *cnt)	/* out: number of bytes transfered */
{
	struct ace_softc *sc = (struct ace_softc *)devdata;
	int part = sc->sc_part;
	struct partition *pp = &sc->sc_label.d_partitions[part];
	int s;
	uint32_t sector;

#if 0 //useless?
    if (rw != F_READ)
        return (EINVAL);
#endif

	/*
	 * Partial-block transfers not handled.
	 */
	if (reqcnt & (DEV_BSIZE - 1)) {
		*cnt = 0;
		return (EINVAL);
	}

    /*
     * Compute starting sector
     */
	sector = bn;
	sector += pp->p_offset;
	if (pp->p_fstype == FS_RAID)
		sector += RF_PROTECTED_SECTORS;

    /* read */
    s = SysAce_read(sc->sc_dp,addr,sector,reqcnt >> CF_SECBITS);

	if (s < 0)
		return (EIO);

    /* BUGBUG there's no validation we don't fall off the deep end */
	*cnt = reqcnt;
	return (0);
}

