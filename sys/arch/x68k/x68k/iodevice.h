/*	$NetBSD: iodevice.h,v 1.9.32.1 2007/07/11 20:03:09 mjf Exp $	*/

/*
 * Copyright (c) 1993, 1994, 1995 Masaru Oki
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
 *      This product includes software developed by Masaru Oki.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

/*
 * custom CRTC
 */
struct crtc {
	unsigned short r00, r01, r02, r03, r04, r05, r06, r07;
	unsigned short r08, r09, r10, r11, r12, r13, r14, r15;
	unsigned short r16, r17, r18, r19, r20, r21, r22, r23;
	char pad0[0x0450];
	unsigned short crtctrl;
	char pad1[0x1b7e];
};

/*
 * custom VIDEO Controller
 */
struct videoc {
	unsigned short r0;
	char pad1[0x00fe]; unsigned short r1;
	char pad2[0x00fe]; unsigned short r2;
	char pad3[0x19fe];
};

/*
 * Hitachi HD63450 DMA chip
 */
struct dmac {
			unsigned char csr;
			unsigned char cer;
	char pad0[2];	unsigned char dcr;
			unsigned char ocr;
			unsigned char scr;
			unsigned char ccr;
	char pad1[2];	unsigned short mtc;
			unsigned long mar;
	char pad2[4];	unsigned long dar;
	char pad3[2];	unsigned short btc;
			unsigned long bar;
	char pad4[5];	unsigned char niv;
	char pad5[1];	unsigned char eiv;
	char pad6[1];	unsigned char mfc;
	char pad7[3];	unsigned char cpr;
	char pad8[3];	unsigned char dfc;
	char pad9[7];	unsigned char bfc;
	char pada[5];	unsigned char gcr;
};

/*
 * MC68901 Multi Function Periferal
 */
struct mfp {
	char pad00; unsigned char gpip;
#define MFP_GPIP_HSYNC 0x80
#define MFP_GPIP_VDISP 0x10
	char pad01; unsigned char aer;
	char pad02; unsigned char ddr;
	char pad03; unsigned char iera;
#define MFP_TIMERA_STOP  0
#define MFP_TIMERA_RESET 0x10
	char pad04; unsigned char ierb;
#define MFP_TIMERB_STOP  0
#define MFP_TIMERB_RESET 0x10
	char pad05; unsigned char ipra;
	char pad06; unsigned char iprb;
	char pad07; unsigned char isra;
	char pad08; unsigned char isrb;
	char pad09; unsigned char imra;
	char pad0a; unsigned char imrb;
	char pad0b; unsigned char vr;
	char pad0c; unsigned char tacr;
	char pad0d; unsigned char tbcr;
	char pad0e; unsigned char tcdcr;
	char pad0f; unsigned char tadr;
	char pad10; unsigned char tbdr;
	char pad11; unsigned char tcdr;
	char pad12; unsigned char tddr;
	char pad13; unsigned char scr;
	char pad14; unsigned char ucr;
#define MFP_UCR_EVENP		0x02
#define MFP_UCR_PARENB		0x04
#define MFP_UCR_SYNCMODE	0x00
#define MFP_UCR_ONESB		0x08
#define MFP_UCR_1P5SB		0x10
#define MFP_UCR_TWOSB		0x18
#define MFP_UCR_RW_5		0x60
#define MFP_UCR_RW_6		0x40
#define MFP_UCR_RW_7		0x20
#define MFP_UCR_RW_8		0x00
#define MFP_UCR_CLKX16		0x80
	char pad15; unsigned char rsr;
#define MFP_RSR_BF		0x80
#define MFP_RSR_OE		0x40
#define MFP_RSR_PE		0x20
#define MFP_RSR_FE		0x10
#define MFP_RSR_SS		0x02
#define MFP_RSR_RE		0x01
	char pad16; unsigned char tsr;
#define MFP_TSR_BE		0x80
#define MFP_TSR_TE		0x01
	char pad17; unsigned char udr;
	char pad[0x1fd0];
};

/*
 * RICOH Real Time Clock RP5C15
 */
union rtc {
	struct {
		char pad0; unsigned char sec;
		char pad1; unsigned char sec10;
		char pad2; unsigned char min;
		char pad3; unsigned char min10;
		char pad4; unsigned char hour;
		char pad5; unsigned char hour10;
		char pad6; unsigned char week;
		char pad7; unsigned char day;
		char pad8; unsigned char day10;
		char pad9; unsigned char mon;
		char pada; unsigned char mon10;
		char padb; unsigned char year;
		char padc; unsigned char year10;
		char padd; unsigned char mode;
		char pade; unsigned char test;
		char padf; unsigned char reset;
		char pad[0x1fe0];
	} bank0;
	struct {
		char pad0; unsigned char clkout;
		char pad1; unsigned char adjust;
		char pad2; unsigned char al_min;
		char pad3; unsigned char al_min10;
		char pad4; unsigned char al_hour;
		char pad5; unsigned char al_hour10;
		char pad6; unsigned char al_week;
		char pad7; unsigned char al_day;
		char pad8; unsigned char al_day10;
		char pad9; unsigned char unused;
		char pada; unsigned char ampm;
		char padb; unsigned char leep;
		char padc; unsigned char unused2;
		char padd; unsigned char mode;
		char pade; unsigned char test;
		char padf; unsigned char reset;
		char pad[0x1fe0];
	} bank1;
};

/*
 * Centronics printer port (output only)
 */
struct centro {
	char pad0; unsigned char data;
	char pad1; unsigned char strobe;
	char pad[0x1ffc];
};

/*
 * system control port
 */
struct sysport {
	char pad0; unsigned char contrast;
	char pad1; unsigned char tvctrl;
	char pad2; unsigned char imageunit;
	char pad3; unsigned char keyctrl;
	char pad4; unsigned char waitctrl; /* XXX: X68030 only */
	char pad5; unsigned char mpustat;
	char pad6; unsigned char sramwp;
	char pad7; unsigned char powoff;
	char pad[0x1ff0];
};

/*
 * YAMAHA (Operator type-M) chip.
 */
struct opm {
	char pad0; unsigned char reg;
	char pad1; unsigned char data;
	char pad[0x1ffc];
};

/*
 * OKI MSM6258V ADPCM chip.
 */
struct adpcm {
	char pad0; unsigned char stat;
	char pad1; unsigned char data;
	char pad[0x1ffc];
};

/*
 * NEC 72065 Floppy Disk Controller
 */
struct fdc {
	char pad0; unsigned char stat;
	char pad1; unsigned char data;
	char pad2; unsigned char drvstat;
	char pad3; unsigned char select;
	char pad[0x1ff8];
};

/*
 * FUJITSU SCSI Protocol Controller MB89352
 */
struct spc {
	char pad00; unsigned char bdid;
	char pad02; unsigned char sctl;
	char pad04; unsigned char scmd;
	char pad06; unsigned char tmod;
	char pad08; unsigned char ints;
	char pad0a; unsigned char psns;
	char pad0c; unsigned char ssts;
	char pad0e; unsigned char serr;
	char pad10; unsigned char pctl;
	char pad12; unsigned char mbc;
	char pad14; unsigned char dreg;
	char pad16; unsigned char temp;
	char pad18; unsigned char tch;
	char pad1a; unsigned char tcm;
	char pad1c; unsigned char tcl;
	char pad1e;
	char pad1f;
};

/*
 * Zilog scc.
 */
struct zschan {
	unsigned char	zc_xxx0;
	unsigned char	zc_csr;		/* control and status, and indirect access */
	unsigned char	zc_xxx1;
	unsigned char	zc_data;	/* data */
};

struct zsdevice {
	/* Yes, they are backwards. */
	struct zschan zs_chan_b;
	struct zschan zs_chan_a;
	char pad4; unsigned char bstat;   /* external only        : 2 bytes */
	char pad[6];			  /* ---                  : 6 bytes */
};

struct ppi8255 {
	char pad0; unsigned char porta;
	char pad1; unsigned char portb;
	char pad2; unsigned char portc;
	char pad3; unsigned char ctrl;
	char pad[0x1ff8];
};

struct ioctlr {
	char pad0; unsigned char intr;
	char pad1; unsigned char vect;
	char pad[0x1ffc];
};

/*
 * YAMAHA YM3802 MIDI chip.
 */
struct midi {
	char pad0; unsigned char r00;
	char pad1; unsigned char r01;
	char pad2; unsigned char r02;
	char pad3; unsigned char r03;
	char pad4; unsigned char rn4;
	char pad5; unsigned char rn5;
	char pad6; unsigned char rn6;
	char pad7; unsigned char rn7;
};

#define PHYS_IODEV 0x00C00000

struct IODEVICE
{
	unsigned short gvram[0x100000]; /* 0x00c00000 */
	unsigned char  tvram[0x080000]; /* 0x00e00000 */
	struct crtc io_crtc;		/* 0x00e80000 */
	unsigned short gpalet[0x00100]; /* 0x00e82000 */
	unsigned short tpalet[0x00100]; /* 0x00e82200 */
	struct videoc io_videoc;	/* 0x00e82400 */
	struct dmac io_dma[4];		/* 0x00e84000 */
	char  dmapad[0x1f00];
	char areapad[0x2000];		/* 0x00e86000 */
	struct mfp io_mfp;		/* 0x00e88000 */
	union rtc io_rtc;		/* 0x00e8a000 */
	struct centro io_printer;	/* 0x00e8c000 */
	struct sysport io_sysport;	/* 0x00e8e000 */
	struct opm io_opm;		/* 0x00e90000 */
	struct adpcm io_adpcm;
	struct fdc io_fdc;
	char spcpad1[0x20];		/* 0x00e96000 */
	struct spc io_inspc;		/* 0x00e96020 */
	char spcpad2[0x1fc0];
	struct zsdevice io_inscc;
	char sccpad[0x1ff0];
	struct ppi8255 io_joyport;
	struct ioctlr io_ctlr;		/* 0x00e9c000 */
	char fpcprsv[0x2000];		/* 0x00e9e000 */
	struct spc io_exspc;		/* 0x00ea0000 */
	char exscsirom[0x1fe0];		/* */
	char sysiorsv1[0xda00];		/* 0x00ea2000 */
	struct midi io_midi[2];		/* 0x00eafa00 */
	char sysiorsv2[0x1e0];		/* 0x00eafa20 */
	struct zsdevice io_exscc[4];	/* 0x00eafc00 */
	char sysiorsv3[0x3c0];		/* 0x00eafc40 */
	char sprite[0x10000];		/* 0x00eb0000 */
	char usriorsv1[0xe000];		/* 0x00ec0000 */
	char neptune[0x400];		/* 0x00ece000 */
	char usriorsv2[0x1c00];		/* 0x00ece400 */
	char io_sram[0x10000];		/* 0x00ed0000 */
	char rsv[0x1ff00];		/* 0x00ee0000 */
	char psx16550[0x00020];		/* 0x00efff00 */
	char rsv2[0x000e0];		/* 0x00efff20 */
	char cgrom0_16x16[0x05e00];	/* 0x00f00000 */
	char cgrom1_16x16[0x17800];	/* 0x00f05e00 */
	char cgrom2_16x16[0x1b2c0];	/* 0x00f1d600 */
	char cgrom__rsv1 [0x01740];	/* 0x00f388c0 */
	char cgrom0_8x8  [0x00800];	/* 0x00f3a000 */
	char cgrom0_8x16 [0x01000];	/* 0x00f3a800 */
	char cgrom0_12x12[0x01800];	/* 0x00f3b800 */
	char cgrom0_12x24[0x03000];	/* 0x00f3d000 */
	char cgrom0_24x24[0x0d380];	/* 0x00f40000 */
	char cgrom1_24x24[0x34e00];	/* 0x00f4d380 */
	char cgrom2_24x24[0x3d230];	/* 0x00f82180 */
	char cgrom__rsv2 [0x00c50];	/* 0x00fbf3b0 */
	char inscsirom[0x2000];		/* 0x00fc0000 */
};

#ifdef _KERNEL
#ifndef LOCORE
extern volatile struct IODEVICE *IODEVbase;
#endif

#define mfp     (IODEVbase->io_mfp)
#define	printer	(IODEVbase->io_printer)
#define sysport (IODEVbase->io_sysport)
#define OPM     (IODEVbase->io_opm)
#define adpcm   (IODEVbase->io_adpcm)
#define PPI	(IODEVbase->io_joyport)
#define	ioctlr	(IODEVbase->io_ctlr)
#endif

#if 0
/*
 * devices that need to configure before console use this
 * *and know it* (i.e. everything is really tight certain params won't be
 * passed in some cases and the devices will deal with it)
 */
#include <sys/device.h>
int x68k_config_found(struct cfdata *, struct device *, void *, cfprint_t);
#endif
