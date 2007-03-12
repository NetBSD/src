/*
 * Copyright (c) 1982, 1990 The Regents of the University of California.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)device.h	7.3 (Berkeley) 5/7/91
 *	$Id: device.h,v 1.12.8.1 2007/03/12 05:46:38 rmind Exp $
 */

struct driver {
	int	(*d_init)	(void *);	/* amiga_device or amiga_ctrl */
	char	*d_name;
	int	(*d_start)	(int unit);
	int	(*d_go)		(int unit, ...);
	int	(*d_intr)	(int unit, int stat);
	int	(*d_done)	(int unit);
	int	(*d_ustart)	(int unit);
	int	(*d_req)	();
	void	(*d_free)	();
	void	(*d_reset)	(int unit);
	void	(*d_delay)	(int delay);
	int	(*d_tur)	(int ctlr, int slave, int unit);
	int	(*d_ssu)	(int ctlr, int slave, int unit, int start);
	int	(*d_rqs)	();
	int	(*d_immcmd)	();
	int	(*d_immcmd_nd)	();
	int	(*d_ttread)	();
	int	(*d_ttwrite)	();
	int	(*d_ttoddio)	();
};

struct amiga_ctlr {
	struct driver	*amiga_driver;
	int		amiga_unit;
	int		amiga_alive;
	char		*amiga_addr;
	int		amiga_flags;
	int		amiga_ipl;
};

struct amiga_device {
	struct driver	*amiga_driver;
	struct driver	*amiga_cdriver;
	int		amiga_unit;
	int		amiga_ctlr;
	int		amiga_slave;
	char		*amiga_addr;
	int		amiga_dk;
	int		amiga_flags;
	int		amiga_alive;
	int		amiga_ipl;
	int		amiga_serno;
	int		amiga_size;
};

struct	devqueue {
	struct	devqueue *dq_forw;
	struct	devqueue *dq_back;
	int	dq_ctlr;
	int	dq_unit;
	int	dq_slave;
	struct	driver *dq_driver;
};

#define	MAXCTLRS	16	/* Size of HW table (arbitrary) */
#define	MAXSLAVES	8	/* Slaves per controller (SCSI limit) */

struct amiga_hw {
	void *	hw_pa;		/* physical address of control space */
	int	hw_size;	/* size of control space */
	void *	hw_kva;		/* kernel virtual address of control space */
	int	hw_manufacturer;
	int	hw_product;	/* autoconfig® parameters */
	int	hw_serno;	/* serial number, needed by ethernet boards */
	int	hw_type;
};


/* some I know, some I defined.. PLEASE ADD!! */
#define MANUF_BUILTIN		1
#define PROD_BUILTIN_SCSI	1
#define PROD_BUILTIN_FLOPPY	2
#define PROD_BUILTIN_RS232	3
#define PROD_BUILTIN_CLOCK	4
#define PROD_BUILTIN_KEYBOARD	5
#define PROD_BUILTIN_PPORT	6
#define PROD_BUILTIN_DISPLAY	7
#define PROD_BUILTIN_MOUSE	8
#define PROD_BUILTIN_CLOCK2	9
#define PROD_BUILTIN_FLOP	10

/* They have more than one manuf-id */
#define MANUF_CBM_1		513
#define PROD_CBM_1_A2088	1
#define MANUF_CBM_2		514
#define PROD_CBM_2_A2091	3
#define PROD_CBM_2_A2065	0x70

/* Ameristar Ethernet board is the same as commo's A2065 (dunno exact boardname) */
#define MANUF_AMERISTAR		1053
#define PROD_AMERISTAR_ETHER	1

/* Unilowell tiga-board */
#define MANUF_UNILOWELL		1030
#define PROD_UNILOWELL_A2410	0

/* MacroSystem Retina board */
#define MANUF_MACROSYSTEM	0x4754
#define PROD_MACROSYSTEM_RETINA	6

/* GVP */
#define MANUF_GVP		2017
#define	PROD_GVP_SERIES_I	9		/* Impact Series-I SCSI? */
#define PROD_GVP_SERIES_II	11		/* !!!! many things !!! */
#define	PROD_GVP_X_GF40		0x20		/* G-Force 040 w/o SCSI */
#define	PROD_GVP_X_GF40_SCSI	0x30		/* G-Force 040 SCSI */
#define	PROD_GVP_X_COMBO4	0x60		/* Combo 030 Rev 4 w/o SCSI */
#define	PROD_GVP_X_COMBO4_SCSI	0x70		/* Combo 030 Rev 4 SCSI */
#define	PROD_GVP_X_IOEXTEND	0x98		/* I/O Extender */
#define	PROD_GVP_X_GF30		0xa0		/* G-Force 030 w/o SCSI */
#define	PROD_GVP_X_GF30_SCSI	0xb0		/* G-Force 030 SCSI */
#define	PROD_GVP_X_COMBO3	0xe0		/* Combo 030 Rev 3 w/o SCSI */
#define	PROD_GVP_X_COMBO3_SCSI	0xf0		/* Combo 030 Rev 3 SCSI */
#define	PROD_GVP_X_SCSI_II	0xf8		/* Impact Series-II SCSI */
#define PROD_GVP_IV24		32

/* Progressive Peripherals Inc. */
#define MANUF_PPI		2026
#define PROD_PPI_ZEUS		150
#define PROD_PPI_A500_040	187
#define PROD_PPI_A2000_040	105
#define PROD_PPI_A3000_040	1
#define PROD_PPI_MERCURY	0

/* CSA */
#define MANUF_CSA		1058
#define PROD_CSA_MAGNUM		17
#define PROD_CSA_12G		21

/* MicroBotics */
#define MANUF_MICROBOTICS	1010
#define PROD_MICROBOTICS_VXL30	69

/* Supra */
#define MANUF_SUPRA		1056
#define PROD_SUPRA_WORDSYNC_2	12

/* IVS */
#define	MANUF_IVS		2112
#define PROD_IVS_TRUMPCARD	52
#define	PROD_IVS_VECTOR_ACC	242
#define	PROD_IVS_VECTOR		243

/* Hacker */
#define MANUF_HACKER		2011
#define PROD_HACKER_MLH		1	/* XXXX Michael's scsi */

/* bus types */
#define	B_MASK		0xE000
#define	B_BUILTIN	0x2000
#define B_ZORROII	0x4000
#define B_ZORROIII	0x6000
/* controller types */
#define	C_MASK		0x8F
#define C_FLAG		0x80
#define	C_FLOPPY	0x81
#define C_SCSI		0x82
/* device types (controllers with no slaves) */
#define D_MASK		0x8F
#define	D_BITMAP	0x01
#define	D_LAN		0x02
#define	D_FPA		0x03
#define	D_KEYBOARD	0x04
#define	D_COMMSER	0x05
#define	D_PPORT		0x06
#define D_CLOCK		0x07
#define D_FLOP		0x0a
#define	D_MISC		0x7F

#define HW_ISCTLR(hw)	((hw)->hw_type & C_FLAG)
#define HW_ISFLOPPY(hw)	(((hw)->hw_type & C_MASK) == C_FLOPPY)
#define HW_ISSCSI(hw)	(((hw)->hw_type & C_MASK) == C_SCSI)
#define HW_ISDEV(hw,d)	(((hw)->hw_type & D_MASK) == (d))

/* doesn't belong here... */
/*
 * Pseudo-device attach information (function + number of pseudo-devs).
 */
struct pdevinit {
	void	(*pdev_attach) __P((int));
	int	pdev_count;
};

#ifdef KERNEL
extern struct amiga_hw sc_table[];
extern struct amiga_ctlr amiga_cinit[];
extern struct amiga_device amiga_dinit[];
extern void *sctova(), sctopa(), iomap();
#endif
