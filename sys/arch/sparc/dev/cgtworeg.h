/*	$NetBSD: cgtworeg.h,v 1.2 1995/10/02 09:07:03 pk Exp $ */

/*
 * Copyright (c) 1994 Dennis Ferguson
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

/* cg2reg.h - CG2 colour frame buffer definitions
 *
 * The mapped memory looks like:
 *
 *  offset     contents
 * 0x000000  bit plane map - 1st (of 8) plane used by the X server in -mono mode
 * 0x100000  pixel map - used by the X server in color mode
 * 0x200000  raster op mode memory map - unused by X server
 * 0x300000  random control registers (lots of spaces in between)
 * 0x310000  shadow colour map
 */

/* Frame buffer memory size and depth */
#define	CG2_FBSIZE	(1024 * 1024)
#define	CG2_N_PLANE	8

/* Screen dimensions */
#define	CG2_WIDTH	1152
#define	CG2_HEIGHT	900

/* Colourmap size */
#define CG2_CMSIZE	256

#define CG2_BITPLANE_OFF	0
#define CG2_BITPLANE_SIZE	0x100000
#define CG2_PIXMAP_OFF		(CG2_BITPLANE_OFF + CG2_BITPLANE_SIZE)
#define CG2_PIXMAP_SIZE		0x100000
#define CG2_ROPMEM_OFF		(CG2_PIXMAP_OFF + CG2_PIXMAP_SIZE)
#define CG2_ROPMEM_SIZE		0x100000
#define CG2_CTLREG_OFF		(CG2_ROPMEM_OFF + CG2_ROPMEM_SIZE)
#define CG2_CTLREG_SIZE		0x010600
#define CG2_MAPPED_SIZE		(CG2_CTLREG_OFF + CG2_CTLREG_SIZE)


/*
 * Control/status register.  The X server only appears to use update_cmap
 * and video_enab.
 */
struct cg2statusreg {
	u_int reserved : 2;	/* not used */
        u_int fastread : 1;	/* r/o: has some feature I don't understand */
        u_int id : 1;		/* r/o: ext status and ID registers exist */
        u_int resolution : 4;	/* screen resolution, 0 means 1152x900 */
        u_int retrace : 1;	/* r/o: retrace in progress */
        u_int inpend : 1;	/* r/o: interrupt request */
        u_int ropmode : 3;	/* ?? */
        u_int inten : 1;	/* interrupt enable (for end of retrace) */
        u_int update_cmap : 1;	/* copy/use shadow colour map */
        u_int video_enab : 1;	/* enable video */
};


/*
 * Extended status register.  Unused by X server
 */
struct cg2_extstatus {
	u_int gpintreq : 1;	/* interrupt request */
	u_int gpintdis : 1;	/* interrupt disable */
	u_int reserved : 13;	/* unused */
	u_int gpbus : 1;	/* bus enabled */
};


/*
 * CG2 control. I dropped many details from the sun3 description
 * (see sun3/include/cg2reg.h) which will likely never surface
 * in the kernel driver.
 */
struct cg2reg {
	char	pad1[4096][9];

	struct	cg2statusreg status;
	char	pad2[4096-sizeof(struct cg2statusreg)];

	short	planemask;
	char	pad3[4096-sizeof(short)];

	char	pad4[16384];

	short	interruptvec;
	char	pad5[32-sizeof(short)];

	short	board_id;
	char	pad6[16-sizeof(short)];

	struct	cg2_extstatus extstatus;
	char	pad7[16-sizeof(short)];

	char	pad8[4032];

	u_short	redmap[256];	/* shadow colour maps */
	u_short	greenmap[256];
	u_short	bluemap[256];
};
