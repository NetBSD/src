/*
 *	iocs.h
 *	X680x0 IOCS call interface
 *
 *	written by Yasha (ITOH Yasufumi)
 *	(based on PD libc 1.1.32 by PROJECT C Library)
 *	public domain
 *
 *	$NetBSD: iocs.h,v 1.4.64.1 2008/01/02 21:51:16 bouyer Exp $
 */
/*
 * PROJECT C Library, X68000 PROGRAMMING INTERFACE DEFINITION
 * --------------------------------------------------------------------
 * This file is written by the Project C Library Group,  and completely
 * in public domain. You can freely use, copy, modify, and redistribute
 * the whole contents, without this notice.
 * --------------------------------------------------------------------
 * Id: iocs.h,v 1.1.1.1 1993/04/18 16:14:27 mura Exp
 * Id: iocs_p.h,v 1.5 1993/10/06 16:46:12 mura Exp
 * Id: scsi.h,v 1.3 1994/07/31 17:21:50 mura Exp
 */

#ifndef __X68K_IOCS_H__
#define __X68K_IOCS_H__

#include <sys/cdefs.h>

struct iocs_boxptr {
	short		x1;
	short		y1;
	short		x2;
	short		y2;
	unsigned short	color;
	unsigned short	linestyle;
};

struct iocs_circleptr {
	short		x;
	short		y;
	unsigned short	radius;
	unsigned short	color;
	short		start;
	short		end;
	unsigned short	ratio;
};

struct iocs_fillptr {
	short		x1;
	short		y1;
	short		x2;
	short		y2;
	unsigned short	color;
};

struct iocs_fntbuf {
	short		xl;
	short		yl;
	unsigned char	buffer[72];
};

struct iocs_getptr {
	short	x1;
	short	y1;
	short	x2;
	short	y2;
	void	*buf_start;
	void	*buf_end;
};

struct iocs_lineptr {
	short		x1;
	short		y1;
	short		x2;
	short		y2;
	unsigned short	color;
	unsigned short	linestyle;
};

struct iocs_paintptr {
	short		x;
	short		y;
	unsigned short	color;
	void		*buf_start;
	void		*buf_end;
} __attribute__((__packed__));

struct iocs_pointptr {
	short		x;
	short		y;
	unsigned short	color;
};

struct iocs_psetptr {
	short		x;
	short		y;
	unsigned short	color;
};

struct iocs_putptr {
	short		x1;
	short		y1;
	short		x2;
	short		y2;
	const void	*buf_start;
	const void	*buf_end;
};

struct iocs_symbolptr {
	short		x1;
	short		y1;
	unsigned char	*string_address;
	unsigned char	mag_x;
	unsigned char	mag_y;
	unsigned short	color;
	unsigned char	font_type;
	unsigned char	angle;
} __attribute__((__packed__));

struct iocs_regs {
	int	d0;
	int	d1;
	int	d2;
	int	d3;
	int	d4;
	int	d5;
	int	d6;
	int	d7;
	int	a1;
	int	a2;
	int	a3;
	int	a4;
	int	a5;
	int	a6;
};

struct iocs_chain {
	void		*addr;
	unsigned short	len;
} __attribute__((__packed__));

struct iocs_chain2 {
	void			*addr;
	unsigned short		len;
	const struct iocs_chain2 *next;
} __attribute__((__packed__));

struct iocs_clipxy {
	short	xs;
	short	ys;
	short	xe;
	short	ye;
};

struct iocs_patst {
	short	offsetx;
	short	offsety;
	short	shadow[16];
	short	pattern[16];
};

struct iocs_tboxptr {
	unsigned short	vram_page;
	short		x;
	short		y;
	short		x1;
	short		y1;
	unsigned short	line_style;
};

struct iocs_txfillptr {
	unsigned short	vram_page;
	short		x;
	short		y;
	short		x1;
	short		y1;
	unsigned short	fill_patn;
};

struct iocs_trevptr {
	unsigned short	vram_page;
	short		x;
	short		y;
	short		x1;
	short		y1;
};

struct iocs_xlineptr {
	unsigned short	vram_page;
	short		x;
	short		y;
	short		x1;
	unsigned short	line_style;
};

struct iocs_ylineptr {
	unsigned short	vram_page;
	short		x;
	short		y;
	short		y1;
	unsigned short	line_style;
};

struct iocs_tlineptr {
	unsigned short	vram_page;
	short		x;
	short		y;
	short		x1;
	short		y1;
	unsigned short	line_style;
};

/*
 * for SCSI calls
 */

struct iocs_readcap {
	unsigned long	block;
	unsigned long	size;
};

struct iocs_inquiry {
	unsigned char	unit;
	unsigned char	info;
	unsigned char	ver;
	unsigned char	reserve;
	unsigned char	size;
	unsigned char	buff[0];	/* actually longer */
};

/*
 * arguments:
 *	dn		32bit arg -> 32bit data reg (input)
 *	an		32bit arg -> 32bit addr reg (input)
 *	odn		32bit arg -> 32bit addr (location to store dn value)
 *	dn=(num)	(no C arg) -> load immediate value to the data register
 *	dn=ww		two 32bit args -> (LSWord #1) << 16 | (LSWord #2)
 *	dn=bb		two 32bit args -> (LSByte #1) << 8 | (LSByte #2)
 *	dn=hsv		three args (H S V) -> encode HSV values in dn
 * opts:
 *	retd2		return value is d2 of IOCS call
 *	err_d0		nonzero d0 is an error -- skip storing values
 *	noret		the IOCS call never returns
 *	c_md		special for IOCS_CACHE_MD
 *	b_super		special for IOCS_B_SUPER
 *	sp_regst	special for IOCS_SP_REGST
 *	b_curmod	special for IOCS_B_CURMOD
 *	b_curpat	special for IOCS_B_CURPAT
 *	b_scroll	special for IOCS_B_SCROLL
 *	trap15		special for IOCS_TRAP15
 */

/* (none) ; trap15 */	int IOCS_TRAP15 __P((const struct iocs_regs *, struct iocs_regs *));
/* 00 */	int IOCS_B_KEYINP __P((void));
/* 01 */	int IOCS_B_KEYSNS __P((void));
/* 02 */	int IOCS_B_SFTSNS __P((void));
/* 04 d1 */	int IOCS_BITSNS __P((int));
/* 05 d1 */	void IOCS_SKEYSET __P((int));
/* 0c d1 */	void IOCS_TVCTRL __P((int));
/* 0d d1 d2 */	void IOCS_LEDMOD __P((int, int));
/* 0e d1 d2 */	int IOCS_TGUSEMD __P((int, int));
/* 0f d1=ww a1 */	int IOCS_DEFCHR __P((int, int, const void *));
/* 10 d1 */	int IOCS_CRTMOD __P((int));
/* 11 d1 */	int IOCS_CONTRAST __P((int));
/* 12 d1=hsv */	int __pure IOCS_HSVTORGB __P((int, int, int)) __attribute__((const));
/* 13 d1 d2 */	int IOCS_TPALET __P((int, int));
/* 14 d1 d2 */	int IOCS_TPALET2 __P((int, int));
/* 15 d1 */	void IOCS_TCOLOR __P((int));
/* 19 d1=ww a1 */	int IOCS_FNTGET __P((int, int, struct iocs_fntbuf *));
/* 1a d1 d2 a1 */	void IOCS_TEXTGET __P((int, int, struct iocs_fntbuf *));
/* 1b d1 d2 a1 */	void IOCS_TEXTPUT __P((int, int, const struct iocs_fntbuf *));
/* 1c d1 d2 a1 a2 */	void IOCS_CLIPPUT __P((int, int, const struct iocs_fntbuf *, const struct iocs_clipxy *));
/* 1d d1 d2 d3 */	void IOCS_SCROLL __P((int, int, int));
/* 1e */	void IOCS_B_CURON __P((void));
/* 1f */	void IOCS_B_CUROFF __P((void));
/* 20 d1 */	int IOCS_B_PUTC __P((int));
/* 21 a1 */	int IOCS_B_PRINT __P((const char *));
/* 22 d1 */	int IOCS_B_COLOR __P((int));
/* 23 d1 d2 */	int IOCS_B_LOCATE __P((int, int));
/* 24 */	void IOCS_B_DOWN_S __P((void));
/* 25 */	void IOCS_B_UP_S __P((void));
/* 26 d1 */	void IOCS_B_UP __P((int));
/* 27 d1 */	void IOCS_B_DOWN __P((int));
/* 28 d1 */	void IOCS_B_RIGHT __P((int));
/* 29 d1 */	void IOCS_B_LEFT __P((int));
/* 2a d1=0 */	void IOCS_B_CLR_ED __P((void));
/* 2a d1=1 */	void IOCS_B_CLR_ST __P((void));
/* 2a d1=2 */	void IOCS_B_CLR_AL __P((void));
/* 2b d1=0 */	void IOCS_B_ERA_ED __P((void));
/* 2b d1=1 */	void IOCS_B_ERA_ST __P((void));
/* 2b d1=2 */	void IOCS_B_ERA_AL __P((void));
/* 2c d1 */	void IOCS_B_INS __P((int));
/* 2d d1 */	void IOCS_B_DEL __P((int));
/* 2e d1=ww d2=ww ; retd2 */	int IOCS_B_CONSOL __P((int, int, int, int));
/* 2f d1 d2 d3 d4 a1 ; retd2 */	int IOCS_B_PUTMES __P((int, int, int, int, const char *));
/* 30 d1 */	int IOCS_SET232C __P((int));
/* 31 */	int IOCS_LOF232C __P((void));
/* 32 */	int IOCS_INP232C __P((void));
/* 33 */	int IOCS_ISNS232C __P((void));
/* 34 */	int IOCS_OSNS232C __P((void));
/* 35 d1 */	void IOCS_OUT232C __P((int));
/* 3b d1 */	int IOCS_JOYGET __P((int));
/* 3c d1=bb */	int IOCS_INIT_PRN __P((int, int));
/* 3d */	int IOCS_SNSPRN __P((void));
/* 3e d1 */	void IOCS_OUTLPT __P((int));
/* 3f d1 */	void IOCS_OUTPRN __P((int));
/* 40 d1 d2 */	int IOCS_B_SEEK __P((int, int));
/* 41 d1 d2 d3 a1 */	int IOCS_B_VERIFY __P((int, int, int, const void *));
/* 42 d1 d2 d3 a1 */	int IOCS_B_READDI __P((int, int, int, void *));
/* 43 d1 a1 d2 */	int IOCS_B_DSKINI __P((int, const void *, int));
/* 44 d1 */	int IOCS_B_DRVSNS __P((int));
/* 45 d1 d2 d3 a1 */	int IOCS_B_WRITE __P((int, int, int, const void *));
/* 46 d1 d2 d3 a1 */	int IOCS_B_READ __P((int, int, int, void *));
/* 47 d1 */	int IOCS_B_RECALI __P((int));
/* 48 d1 d2 d3 a1 */	int IOCS_B_ASSIGN __P((int, int, int, const void *));
/* 49 d1 d2 d3 a1 */	int IOCS_B_WRITED __P((int, int, int, const void *));
/* 4a d1 d2 od2 */	int IOCS_B_READID __P((int, int, void *));
/* 4b d1 d2 d3 */	int IOCS_B_BADFMT __P((int, int, int));
/* 4c d1 d2 d3 a1 */	int IOCS_B_READDL __P((int, int, int, void *));
/* 4d d1 d2 d3 a1 */	int IOCS_B_FORMAT __P((int, int, int, const void *));
/* 4e d1 d2 */	int IOCS_B_DRVCHK __P((int, int));
/* 4f d1 */	int IOCS_B_EJECT __P((int));
/* 50 d1 */	int IOCS_BINDATEBCD __P((int));
/* 51 d1 */	void IOCS_BINDATESET __P((int));
/* 52 d1 */	int IOCS_TIMEBCD __P((int));
/* 53 d1 */	void IOCS_TIMESET __P((int));
/* 54 */	int IOCS_BINDATEGET __P((void));
/* 55 d1 */	int IOCS_DATEBIN __P((int));
/* 56 */	int IOCS_TIMEGET __P((void));
/* 57 d1 */	int IOCS_TIMEBIN __P((int));
/* 58 a1 */	int IOCS_DATECNV __P((const char *));
/* 59 a1 */	int IOCS_TIMECNV __P((const char *));
/* 5a d1 a1 */	int IOCS_DATEASC __P((int, char *));
/* 5b d1 a1 */	int IOCS_TIMEASC __P((int, char *));
/* 5c d1 a1 */	void IOCS_DAYASC __P((int, char *));
/* 5d d1 */	int IOCS_ALARMMOD __P((int));
/* 5e d1 d2 a1 */	int IOCS_ALARMSET __P((int, int, int));
/* 5f od1 od2 od0 */	int IOCS_ALARMGET __P((int *, int *, int *));
/* 60 a1 d1 d2 */	void IOCS_ADPCMOUT __P((const void *, int, int));
/* 61 a1 d1 d2 */	void IOCS_ADPCMINP __P((void *, int, int));
/* 62 a1 d1 d2 */	void IOCS_ADPCMAOT __P((const struct iocs_chain *, int, int));
/* 63 a1 d1 d2 */	void IOCS_ADPCMAIN __P((const struct iocs_chain *, int, int));
/* 64 a1 d1 */	void IOCS_ADPCMLOT __P((const struct iocs_chain2 *, int));
/* 65 a1 d1 */	void IOCS_ADPCMLIN __P((const struct iocs_chain2 *, int));
/* 66 */	int IOCS_ADPCMSNS __P((void));
/* 67 d1 */	void IOCS_ADPCMMOD __P((int));
/* 68 d1 d2 */	void IOCS_OPMSET __P((int, int));
/* 69 */	int IOCS_OPMSNS __P((void));
/* 6a a1 */	int IOCS_OPMINTST __P((const void *));
/* 6b a1 d1=bb */	int IOCS_TIMERDST __P((const void *, int, int));
/* 6c a1 d1=bb */	int IOCS_VDISPST __P((const void *, int, int));
/* 6d a1 d1 */	int IOCS_CRTCRAS __P((const void *, int));
/* 6e a1 */	int IOCS_HSYNCST __P((const void *));
/* 6f a1 */	int IOCS_PRNINTST __P((const void *));
/* 70 */	void IOCS_MS_INIT __P((void));
/* 71 */	void IOCS_MS_CURON __P((void));
/* 72 */	void IOCS_MS_CUROF __P((void));
/* 73 */	int IOCS_MS_STAT __P((void));
/* 74 */	int IOCS_MS_GETDT __P((void));
/* 75 */	int IOCS_MS_CURGT __P((void));
/* 76 d1=ww */	int IOCS_MS_CURST __P((int, int));
/* 77 d1=ww d2=ww */	int IOCS_MS_LIMIT __P((int, int, int, int));
/* 78 d1 d2 */	int IOCS_MS_OFFTM __P((int, int));
/* 79 d1 d2 */	int IOCS_MS_ONTM __P((int, int));
/* 7a d1 a1 */	void IOCS_MS_PATST __P((int, const struct iocs_patst *));
/* 7b d1 */	void IOCS_MS_SEL __P((int));
/* 7c a1 */	void IOCS_MS_SEL2 __P((const short *));
/* 7d d1 d2=ww */	int IOCS_SKEY_MOD __P((int, int, int));
/* 7e */	void IOCS_DENSNS __P((void));
/* 7f */	int IOCS_ONTIME __P((void));
/* 80 d1 a1 */	int IOCS_B_INTVCS __P((int, int));
/* 81 ; b_super */	int IOCS_B_SUPER __P((int));
/* 82 a1 */	int IOCS_B_BPEEK __P((const void *));
/* 83 a1 */	int IOCS_B_WPEEK __P((const void *));
/* 84 a1 */	int IOCS_B_LPEEK __P((const void *));
/* 85 a1 a2 d1 */	void IOCS_B_MEMSTR __P((const void *, void *, int));
/* 86 a1 d1 */	void IOCS_B_BPOKE __P((void *, int));
/* 87 a1 d1 */	void IOCS_B_WPOKE __P((void *, int));
/* 88 a1 d1 */	void IOCS_B_LPOKE __P((void *, int));
/* 89 a1 a2 d1 */	void IOCS_B_MEMSET __P((void *, const void *, int));
/* 8a a1 a2 d1 d2 */	void IOCS_DMAMOVE __P((void *, void *, int, int));
/* 8b a1 a2 d1 d2 */	void IOCS_DMAMOV_A __P((const struct iocs_chain *, void *, int, int));
/* 8c a1 a2 d1 */	void IOCS_DMAMOV_L __P((const struct iocs_chain2 *, void *, int));
/* 8d */	int IOCS_DMAMODE __P((void));
/* 8e */	int __pure IOCS_BOOTINF __P((void)) __attribute__((const));
/* 8f */	int __pure IOCS_ROMVER __P((void)) __attribute__((const));
/* 90 */	void IOCS_G_CLR_ON __P((void));
/* 94 d1 d2 */	int IOCS_GPALET __P((int, int));
/* a0 d1 */	int __pure IOCS_SFTJIS __P((int)) __attribute__((const));
/* a1 d1 */	int __pure IOCS_JISSFT __P((int)) __attribute__((const));
/* a2 d1=ww */	int __pure IOCS_AKCONV __P((int, int)) __attribute__((const));
/* a3 d1 a1 a2 */	int IOCS_RMACNV __P((int, char *, char *));
/* a4 a1 */	int IOCS_DAKJOB __P((char *));
/* a5 a1 */	int IOCS_HANJOB __P((char *));
/* ac d1=0 */	int IOCS_MPU_STAT __P((void));		/* ROM 1.3 only */
/* ac d1=1 */	int IOCS_CACHE_ST __P((void));		/* ROM 1.3 only */
/* ac d1=3 d2 ; c_md */	int IOCS_CACHE_MD __P((int));	/* ROM 1.3 only */
/* ad d1 ; b_curmod */	void IOCS_B_CURMOD __P((int));		/*1.3/IOCS.X*/
/* ad d1=2 d2 ; b_curpat */	void IOCS_B_CURPAT __P((int));	/*1.3/IOCS.X*/
/* ad d1=2 d2=0 */	void IOCS_B_CURPAT1 __P((void));	/*1.3/IOCS.X*/
/* ad d1=3 d2 */	void IOCS_B_CURDEF __P((void *));	/*1.3/IOCS.X*/
/* ad d1=16 d2 ; b_scroll */	void IOCS_B_SCROLL __P((int));	/*1.3/IOCS.X*/
/* ae */	void IOCS_OS_CURON __P((void));
/* af */	void IOCS_OS_CUROF __P((void));
/* b0 d1 */	int IOCS_DRAWMODE __P((int));		/* ROM 1.3, IOCS.X */
/* b1 d1 */	int IOCS_APAGE __P((int));
/* b2 d1 */	int IOCS_VPAGE __P((int));
/* b3 d1 d2 d3 */	int IOCS_HOME __P((int, int, int));
/* b4 d1 d2 d3 d4 */	int IOCS_WINDOW __P((int, int, int, int));
/* b5 */	int IOCS_WIPE __P((void));
/* b6 a1 */	int IOCS_PSET __P((const struct iocs_psetptr *));
/* b7 a1 */	int IOCS_POINT __P((const struct iocs_pointptr *));
/* b8 a1 */	int IOCS_LINE __P((const struct iocs_lineptr *));
/* b9 a1 */	int IOCS_BOX __P((const struct iocs_boxptr *));
/* ba a1 */	int IOCS_FILL __P((const struct iocs_fillptr *));
/* bb a1 */	int IOCS_CIRCLE __P((const struct iocs_circleptr *));
/* bc a1 */	int IOCS_PAINT __P((struct iocs_paintptr *));
/* bd a1 */	int IOCS_SYMBOL __P((const struct iocs_symbolptr *));
/* be a1 */	int IOCS_GETGRM __P((struct iocs_getptr *));
/* bf a1 */	int IOCS_PUTGRM __P((const struct iocs_putptr *));
/* c0 */	int IOCS_SP_INIT __P((void));
/* c1 */	int IOCS_SP_ON __P((void));
/* c2 */	void IOCS_SP_OFF __P((void));
/* c3 d1 */	int IOCS_SP_CGCLR __P((int));
/* c4 d1 d2 a1 */	int IOCS_SP_DEFCG __P((int, int, const void *));
/* c5 d1 d2 a1 */	int IOCS_SP_GTPCG __P((int, int, void *));

/*
 * XXX	SP_REGST in XC iocslib:	args: int,      int, int, int, int
 *				      (c6 d1 d2 d3 d4 d5)
 *	XC manual and PD libc:	args: int, int, int, int, int, int
 *				      (c6 d0 d1 d2 d3 d4 d5 ; sp_regst)
 * we use the latter interface...
 */
/* c6 d0 d1 d2 d3 d4 d5 ; sp_regst */	int IOCS_SP_REGST __P((int, int, int, int, int, int));
/* c7 d1 od2 od3 od4 od5 ; err_d0 */	int IOCS_SP_REGGT __P((int, int *, int *, int *, int *));
/* c8 d1 d2 d3 */	int IOCS_BGSCRLST __P((int, int, int));
/* c9 d1 od2 od3 ; err_d0 */	int IOCS_BGSCRLGT __P((int, int *, int *));
/* ca d1 d2 d3 */	int IOCS_BGCTRLST __P((int, int, int));
/* cb d1 */	int IOCS_BGCTRLGT __P((int));
/* cc d1 d2 */	int IOCS_BGTEXTCL __P((int, int));
/* cd d1 d2 d3 d4 */	int IOCS_BGTEXTST __P((int, int, int, int));
/* ce d1 d2 d3 */	int IOCS_BGTEXTGT __P((int, int, int));
/* cf d1 d2 d3 */	int IOCS_SPALET __P((int, int, int));
/* d3 a1 */	void IOCS_TXXLINE __P((const struct iocs_xlineptr *));
/* d4 a1 */	void IOCS_TXYLINE __P((const struct iocs_ylineptr *));
/* d5 a1 */	void IOCS_TXLINE __P((struct iocs_tlineptr)); /* 1.3, IOCS.X */
/* d6 a1 */	void IOCS_TXBOX __P((const struct iocs_tboxptr *));
/* d7 a1 */	void IOCS_TXFILL __P((const struct iocs_txfillptr *));
/* d8 a1 */	void IOCS_TXREV __P((const struct iocs_trevptr *));
/* df d1 d2 d3 */	void IOCS_TXRASCPY __P((int, int, int));
/* fd */	void IOCS_ABORTRST __P((void));
/* fe ; noret */	__dead void IOCS_IPLERR __P((void));
/* ff ; noret */	__dead void IOCS_ABORTJOB __P((void));

/* SCSI calls */
/* f5 d1=0 */		void IOCS_S_RESET __P((void));
/* f5 d1=1 d4 */	int IOCS_S_SELECT __P((int));
/* f5 d1=3 d3 a1 */	int IOCS_S_CMDOUT __P((int, void *));
/* f5 d1=4 d3 a1 */	int IOCS_S_DATAIN __P((int, void *));
/* f5 d1=5 d3 a1 */	int IOCS_S_DATAOUT __P((int, void *));
/* f5 d1=6 a1 */	int IOCS_S_STSIN __P((void *));
/* f5 d1=7 a1 */	int IOCS_S_MSGIN __P((void *));
/* f5 d1=8 a1 */	int IOCS_S_MSGOUT __P((void *));
/* f5 d1=9 */		int IOCS_S_PHASE __P((void));
/* f5 d1=32 d3 d4 a1 */	int IOCS_S_INQUIRY __P((int, int, struct iocs_inquiry *));
/* f5 d1=33 d2 d3 d4 d5 a1 */	int IOCS_S_READ __P((int, int, int, int, void *));
/* f5 d1=34 d2 d3 d4 d5 a1 */	int IOCS_S_WRITE __P((int, int, int, int, void *));
/* f5 d1=35 d3 d4 */	int IOCS_S_FORMAT __P((int, int));
/* f5 d1=36 d4 */	int IOCS_S_TESTUNIT __P((int));
/* f5 d1=37 d4 a1 */	int IOCS_S_READCAP __P((int, struct iocs_readcap *));
/* f5 d1=38 d2 d3 d4 d5 a1 */	int IOCS_S_READEXT __P((int, int, int, int, void *));
/* f5 d1=39 d2 d3 d4 d5 a1 */	int IOCS_S_WRITEEXT __P((int, int, int, int, void *));
/* f5 d1=43 d4 */	int IOCS_S_REZEROUNIT __P((int));
/* f5 d1=44 d3 d4 a1 */	int IOCS_S_REQUEST __P((int, int, void *));
/* f5 d1=45 d2 d4 */	int IOCS_S_SEEK __P((int, int));
/* f5 d1=47 d3 d4 */	int IOCS_S_STARTSTOP __P((int, int));
/* f5 d1=49 d3 d4 a1 */	int IOCS_S_REASSIGN __P((int, int, void *));
/* f5 d1=50 d3 d4 */	int IOCS_S_PAMEDIUM __P((int, int));

#endif /* __X68K_IOCS_H__ */
