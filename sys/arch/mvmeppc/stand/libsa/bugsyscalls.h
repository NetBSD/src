/*	$NetBSD: bugsyscalls.h,v 1.1.14.2 2002/06/23 17:38:39 jdolecek Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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

#ifndef __BUGSYSCALLS_H
#define __BUGSYSCALLS_H

/*
 * Basic Console I/O system calls
 */
extern	char	bugsys_inchr(void);
extern	void	bugsys_outchr(char);
extern	int	bugsys_instat(void);


/*
 * Basic Disk I/O system calls
 */
struct bug_diskio {
	u_int8_t	dc_clun;	/* Controller LUN for operation */
	u_int8_t	dc_dlun;	/* Device LUN for operation */
	u_int16_t	dc_status;	/* Completion status written here */
	void		*dc_buffer;	/* Pointer to dest/src buffer */
	u_int32_t	dc_block;	/* Starting block/file-number */
	u_int16_t	dc_nblocks;	/* Number of blocks to transfer */
	u_int8_t	dc_flag;	/* Flag (see below) */
	u_int8_t	dc_am;		/* VMEbus address modifier, or zero */
};
#define	BUG_DISKCMD_FLAG_FILEMARK	(1 << 7)
#define	BUG_DISKCMD_FLAG_IFN		(1 << 1)
#define	BUG_DISKCMD_FLAG_EOF		(1 << 0)

extern	int	bugsys_dskrd(struct bug_diskio *);
extern	int	bugsys_dskwr(struct bug_diskio *);

/*
 * Basic Nework I/O system calls
 */
struct bug_netio {
	u_int8_t	nc_clun;	/* Controller LUN for operation */
	u_int8_t	nc_dlun;	/* Device LUN for operation */
	u_int16_t	nc_status;	/* Completion status written here */
	u_int32_t	nc_command;	/* Command identifier */
	void		*nc_buffer;	/* Pointer to dest/src data buffer */
	u_int32_t	nc_length;	/* Number of bytes of data */
	u_int32_t	nc_csr;		/* Status/Control flags */
};
#define	BUG_NETIO_CMD_INIT	   0x0	/* Initialise Device/Channel/Node */
#define	BUG_NETIO_CMD_GET_MAC	   0x1	/* Get ethernet MAC address */
#define	BUG_NETIO_CMD_TRANSMIT	   0x2	/* Transmit a packet of data */
#define BUG_NETIO_CMD_RECEIVE	   0x3	/* Receive a packet of data */
#define	BUG_NETIO_CMD_FLUSH	   0x4	/* Flush receive buffers */
#define	BUG_NETIO_CMD_RESET	   0x5	/* Reset Device/Channel/Node */

#define	BUG_NETIO_CSR_RX_COMPLETE  (1<<16)

extern	int	bugsys_netio(struct bug_netio *);

/*
 * Miscellaneous system calls
 */
extern	void	bugsys_delay(int);

struct bug_boardid {
	u_int32_t	bi_eyecatcher;	/* Eye catcher pattern */
	u_int8_t	bi_rev;		/* PPCBug Revision, in BCD */
	u_int8_t	bi_month;	/* PPCBug Month, in BCD */
	u_int8_t	bi_day;		/* PPCBug Day, in BCD */
	u_int8_t	bi_year;	/* PPCBug Year, in BCD */
	u_int16_t	bi_size;	/* Size of this structure */
	u_int16_t	bi_resvd;
	u_int16_t	bi_bnumber;	/* Board number, in BCD */
	u_int16_t	bi_bsuffix;	/* Board suffix, in BCD */
	u_int32_t	bi_options;	/* Board options. See below */
	u_int16_t	bi_clun;	/* Boot device CLUN */
	u_int16_t	bi_dlun;	/* Boot device DLUN */
	u_int16_t	bi_devtype;	/* Boot device type. See below */
	u_int16_t	bi_devnumber;	/* Boot device number */
	u_int32_t	bi_resvd2;
};
#define	BUG_BOARDID_OPT_CPU_MASK	0x0f
#define	BUG_BOARDID_OPT_CPU_SHIFT	0
#define	 BUG_BOARDID_OPT_CPU_MPC620	1
#define	 BUG_BOARDID_OPT_CPU_MPC601	1
#define	 BUG_BOARDID_OPT_CPU_MPC602	2
#define	 BUG_BOARDID_OPT_CPU_MPC603	3
#define	 BUG_BOARDID_OPT_CPU_MPC604	4
#define	BUG_BOARDID_OPT_FAMILY_MASK	0x07
#define	BUG_BOARDID_OPT_FAMILY_SHIFT	6
#define	BUG_BOARDID_OPT_FAMILY_MPC600	2
#define	BUG_BOARDID_OPT_FPC		(1 << 7)
#define	BUG_BOARDID_OPT_MMU		(1 << 8)
#define	BUG_BOARDID_OPT_MMB		(1 << 9)

#define	BUG_BOARDID_DEVTYPE_DIRECT	0  /* Direct Access Device (disk) */
#define	BUG_BOARDID_DEVTYPE_SEQ		1  /* Sequential Access Device (tape) */
#define	BUG_BOARDID_DEVTYPE_PRINTER	2  /* Printer device */
#define	BUG_BOARDID_DEVTYPE_PROCESSOR	3  /* Processor device */
#define	BUG_BOARDID_DEVTYPE_WORM	4  /* WORM device */
#define	BUG_BOARDID_DEVTYPE_CDROM	5  /* CD-ROM device */
#define	BUG_BOARDID_DEVTYPE_SCANNER	6  /* Scanner device */
#define	BUG_BOARDID_DEVTYPE_OPTICAL	7  /* Optical memory device */
#define	BUG_BOARDID_DEVTYPE_CHANGER	8  /* Medium Changer device */
#define	BUG_BOARDID_DEVTYPE_COMMS	9  /* Communications device */

extern	struct bug_boardid	*bugsys_brdid(void);

struct bug_ioinquiry {
	u_int32_t	ii_portnum;	/* Port number */
	char		*ii_boardname;	/* Board name pointer */
	u_int32_t	ii_channel;	/* Channel number */
	u_int32_t	ii_devaddr;	/* Device address */
	u_int32_t	ii_concurmode;	/* Concurrent mode */
	u_int32_t	ii_modemid;	/* Modem ID */
	struct bug_ioctrl *ii_ioctrl;	/* I/O control struction pointer */
	u_int32_t	ii_error;	/* Error code */
	u_int32_t	ii_resvd[3];
};
#define	BUG_IOINQ_PORT_CONSOLE		0xffffffff

struct bug_ioctrl {
	u_int32_t	ic_ctrlbits;
	u_int32_t	ic_baud;
	u_int32_t	ic_protocol;
	u_int32_t	ic_sync1;
	u_int32_t	ic_sync2;
	u_int32_t	ic_xonchar;
	u_int32_t	ic_xoffchar;
};
#define	IOCTRL_PARITY_ODD	(1 << 0)
#define	IOCTRL_PARITY_EVEN	(1 << 1)
#define	IOCTRL_BITS_8		(1 << 2)
#define	IOCTRL_BITS_7		(1 << 3)
#define	IOCTRL_BITS_6		(1 << 4)
#define	IOCTRL_BITS_5		(1 << 5)
#define	IOCTRL_STOP_2		(1 << 6)
#define	IOCTRL_STOP_1		(1 << 7)

extern	struct bug_ioinquiry	*bugsys_ioinq(struct bug_ioinquiry *);

struct bug_rtc_rd {
	u_int8_t	rr_year;
	u_int8_t	rr_month;
	u_int8_t	rr_dayofmonth;
	u_int8_t	rr_dayofweek;
	u_int8_t	rr_hour;
	u_int8_t	rr_minute;
	u_int8_t	rr_second;
	u_int8_t	rr_calibration;
};

extern	void	bugsys_rtc_rd(struct bug_rtc_rd *);

/*
 * Information passed from bug to the bootstrap program when in PReP mode.
 *
 * Note: This is the only option for booting from disk...
 */
struct bug_prepinfo {
	void	*bpi_residual;		/* PReP mode's "residual data" */
	void	*bpi_loadaddr;		/* Load address of bootstrap code */
};

/*
 * Information passed when in traditional PPCBug mode
 *
 * Note: This appears to be the case for netboot only...
 */
struct bug_buginfo {
	int		bbi_clun;	/* Boot controller LUN */
	int		bbi_dlun;	/* Boot device LUN */
	u_int32_t	bbi_devaddr;	/* PCI config. addr. of boot device */
	void		*bbi_loadaddr;	/* Load address of bootstrap code */
	void		*bbi_ipaddr;	/* Pointer to IP address parameters */
	const char	*bbi_fnstart;	/* Boot filename string start */
	const char	*bbi_fnend;	/* Boot filename string end */
	char		*bbi_argstart;	/* Boot argument string start */
	char		*bbi_argend;	/* Boot argument string end */
};

/*
 * Structure passed into C code from srt0.S
 */
struct bug_bootinfo {
	int		bbi_bugmode;	/* Non-zero if traditional bug boot */
	union {
		struct bug_prepinfo bpi;
		struct bug_buginfo bbi;
	} bbi_bi;
};

#endif	/* __BUGSYSCALLS_H */
