/* $NetBSD: nvram.h,v 1.8.14.1 2015/09/22 12:05:50 skrll Exp $ */

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
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

/*
 * Based on the PowerPC Reference Platform NVRAM Specification.
 * Document Number: PPS-AR-FW0002
 * Jan 22, 1996
 * Version 0.3
 */

#ifndef _MACHINE_NVRAM_H
#define _MACHINE_NVRAM_H

#include <sys/ioccom.h>

#if defined(_KERNEL)
/* for the motorola machines */
#include <dev/ic/mk48txxvar.h>
#endif

#define MAX_PREP_NVRAM 0x8000	/* maxmum size of the nvram */

#define NVSIZE 4096		/* standard nvram size */
#define OSAREASIZE 512		/* size of OSArea space */
#define CONFSIZE 1024		/* guess at size of configuration area */

/*
 * The security fields are maintained by the firmware, and should be
 * considered read-only.
 */
typedef struct _SECURITY {
	uint32_t BootErrCnt;	/* count of boot password errors */
	uint32_t ConfigErrCnt;	/* count of config password errors */
	uint32_t BootErrorDT[2];/* Date&Time from RTC of last error in pw */
	uint32_t ConfigErrorDT[2];	/* last config pw error */
	uint32_t BootCorrectDT[2];	/* last correct boot pw */
	uint32_t ConfigCorrectDT[2];	/* last correct config pw */
	uint32_t BootSetDT[2];		/* last set of boot pw */
	uint32_t ConfigSetDT[2];	/* last set of config pw */
	uint8_t Serial[16];	/* Box serial number */
} SECURITY;

typedef enum _ERROR_STATUS {
	Clear = 0,	/* empty entry */
	Pending = 1,
	DiagnosedOK = 2,
	DiagnosedFail = 3,
	Overrun = 4,
	Logged = 5,
} ERROR_STATUS;

typedef enum _OS_ID {
	Unknown = 0,
	Firmware = 1,
	AIX = 2,
	NT = 3,
	WPOS2 = 4,
	WPX = 5,
	Taligent = 6,
	Solaris = 7,
	Netware = 8,
	USL = 9,
	Low_End_Client = 10,
	SCO = 11,
	MK = 12, /* from linux ?? */
} OS_ID;

/*
 * According to IBM, if severity is severe, the OS should not boot.  It should
 * instead run diags.  Umm.. whatever.
 */

typedef struct _ERROR_LOG {
	uint8_t Status;		/* ERROR_STATUS */
	uint8_t Os;		/* OS_ID */
	uint8_t Type;		/* H=hardware S=software */
	uint8_t Severity;	/* S=servere E=Error */
	uint32_t ErrDT[2];	/* date and time from RTC */
	uint8_t code[8];	/* error code */
	union {
		uint8_t detail[20]; /* detail of error */
	} data;
} ERROR_LOG;

typedef enum _BOOT_STATUS {
	BootStarted = 0x01,
	BootFinished = 0x02,
	RestartStarted = 0x04,
	RestartFinished = 0x08,
	PowerFailStarted = 0x10,
	PowerFailFinished = 0x20,
	ProcessorReady = 0x40,
	ProcessorRunning = 0x80,
	ProcessorStart = 0x0100
} BOOT_STATUS;

/*
 * If the OS decided to store data in the os area of NVRAM, this tells us
 * the last user, so we can decide if we want to re-use it or nuke it.
 * I'm not sure what all of these do yet.
 */
typedef struct _RESTART_BLOCK {
	uint16_t Version;
	uint16_t Revision;
	uint32_t BootMasterId;
	uint32_t ProcessorId;
	volatile uint32_t BootStatus;
	uint32_t CheckSum; /* Checksum of RESTART_BLOCK */
	void *RestartAddress;
	void *SaveAreaAddr;
	uint32_t SaveAreaLength;
} RESTART_BLOCK;

typedef enum _OSAREA_USAGE {
	Empty = 0,
	Used = 1,
} OSAREA_USAGE;

typedef enum _PM_MODE {
	Suspend = 0x80, /* part of state is in memory */
	DirtyBit = 0x01, /* used to decide if pushbutton needs to be checked */
	Hibernate = 0, /* nothing is in memory */
} PMMODE;

typedef struct _HEADER {
	uint16_t Size;		/* NVRAM size in K(1024) */
	uint8_t Version;	/* Structure map different */
	uint8_t Revision;	/* Structure map same */
	uint16_t Crc1;		/* checksum from beginning of nvram to OSArea*/
	uint16_t Crc2;		/* cksum of config */
	uint8_t LastOS;		/* OS_ID */
	uint8_t Endian;		/* B if BE L if LE */
	uint8_t OSAreaUSage;	/* OSAREA_USAGE */
	uint8_t PMMode;		/* Shutdown mode */
	RESTART_BLOCK RestartBlock;
	SECURITY Security;
	ERROR_LOG ErrorLog[2];

	/* Global Environment info */
	void *GEAddress;
	uint32_t GELength;
	uint32_t GELastWRiteDT[2]; /* last change to GE area */

	/* Configuration info */
	void *ConfigAddress;
	uint32_t ConfigLength;
	uint32_t ConfigLastWriteDT[2]; /* last change to config area */
	uint32_t ConfigCount;	/* count of entries in configuration */

	/* OS Dependent temp area */
	void *OSAreaAddress;
	uint32_t OSAreaLength;
	uint32_t OSAreaLastWriteDT[2]; /* last change to OSArea */
} HEADER;

typedef struct _NVRAM_MAP {
	HEADER Header;
	uint8_t GEArea[NVSIZE - CONFSIZE - OSAREASIZE - sizeof(HEADER)];
	uint8_t OSArea[OSAREASIZE];
	uint8_t ConfigArea[CONFSIZE];
} NVRAM_MAP;

struct pnviocdesc {
	int	pnv_namelen;	/* len of pnv_name */
	char	*pnv_name;	/* node name */
	int	pnv_buflen;	/* len of pnv_bus */
	char	*pnv_buf;	/* option value result */
	int	pnv_num;	/* number of something */
};

#define DEV_NVRAM	0
#define DEV_RESIDUAL	1

#if defined(_KERNEL)
struct prep_mk48txx_softc {
	device_t *sc_dev;
	bus_space_tag_t sc_bst;	 /* bus tag & handle */
	bus_space_handle_t sc_bsh;      /* */

	struct todr_chip_handle sc_handle; /* TODR handle */
	const char	*sc_model;	/* chip model name */
	bus_size_t	sc_nvramsz;	/* Size of NVRAM on the chip */
	bus_size_t	sc_clkoffset;	/* Offset in NVRAM to clock bits */
	u_int		sc_year0;	/* What year is represented on
					   the system by the chip's year
					   counter at 0 */
	u_int		sc_flag;
#define MK48TXX_NO_CENT_ADJUST  0x0001

	mk48txx_nvrd_t	sc_nvrd;	/* NVRAM/RTC read function */
	mk48txx_nvwr_t	sc_nvwr;	/* NVRAM/RTC write function */
	bus_space_tag_t sc_data;
	bus_space_handle_t sc_datah;
};

struct nvram_pnpbus_softc {
	bus_space_tag_t sc_iot;		/* io space tag */
	bus_space_tag_t sc_as;		/* addr line */
	bus_space_handle_t sc_ash;
	bus_space_handle_t sc_ashs[2];

	bus_space_tag_t sc_data;	/* data line */
	bus_space_handle_t sc_datah;

	/* clock bits for mk48txx */
	struct prep_mk48txx_softc sc_mksc; /* mk48txx softc */

	u_char  sc_open;		/* single use device */
};

#endif /* _KERNEL */

#define PNVIOCGET	_IOWR('O', 1, struct pnviocdesc) /* get var contents */
#define PNVIOCGETNEXTNAME _IOWR('O', 2, struct pnviocdesc) /* get next var */
#define PNVIOCGETNUMGE	_IOWR('O', 3, struct pnviocdesc) /* get nrof vars */
#define PNVIOCSET	_IOW('O', 4, struct pnviocdesc) /* set nvram var */

#endif /* _MACHINE_NVRAM_H */
