/*	$NetBSD: prom.h,v 1.17.122.1 2014/08/20 00:03:14 tls Exp $	*/

/*
 * Copyright (c) 1995 Theo de Raadt
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

#define MVMEPROM_INCHR		0x00
#define MVMEPROM_INSTAT		0x01
#define MVMEPROM_INLN		0x02
#define MVMEPROM_READSTR	0x03
#define MVMEPROM_READLN		0x04
#define MVMEPROM_OUTCHR		0x20
#define MVMEPROM_OUTSTR		0x21
#define MVMEPROM_DSKRD		0x10
#define MVMEPROM_DSKWR		0x11
#define MVMEPROM_DSKCFIG	0x12
#define MVMEPROM_DSKFMT		0x14
#define MVMEPROM_DSKCTRL	0x15
#define MVMEPROM_NETCTRL	0x1d
#define MVMEPROM_OUTSTRCRLF	0x22
#define MVMEPROM_WRITE		0x23
#define MVMEPROM_WRITELN	0x24
#define MVMEPROM_DELAY		0x43
#define MVMEPROM_RTC_RD		0x53
#define MVMEPROM_EXIT		0x63
#define MVMEPROM_GETBRDID	0x70
#define MVMEPROM_ENVIRON	0x71

#define NETCTRLCMD_GETETHER	1

#define ENVIRONCMD_WRITE	1
#define ENVIRONCMD_READ		2
#define ENVIRONTYPE_EOL		0
#define ENVIRONTYPE_START	1
#define ENVIRONTYPE_DISKBOOT	2
#define ENVIRONTYPE_ROMBOOT	3
#define ENVIRONTYPE_NETBOOT	4
#define ENVIRONTYPE_MEMSIZE	5

#ifndef _LOCORE
struct prom_netctrl {
	u_char	dev;
	u_char	ctrl;
	u_short	status;
	u_long	cmd;
	u_long	addr;
	u_long	len;
	u_long	flags;
};

struct prom_environ_hdr {
	u_char	type;
	u_char	len;
};

struct mvmeprom_brdid {
	u_long	eye_catcher;
	u_char	rev;
	u_char	month;
	u_char	day;
	u_char	year;
	u_short	size;
	u_short	rsv1;
	u_short	model;
	u_short	suffix;
	u_short	options;
	u_char	family;
	u_char	cpu;
	u_short	ctrlun;
	u_short	devlun;
	u_short	devtype;
	u_short	devnum;
	u_long	bug;

	/*
	 * XXX: I have seen no documentation for these!
	 *
	 * The following (appears to) exist only on the MVME162 and
	 * upwards. We should figure out what the other fields are.
	 */
	u_char	xx1[16];
	u_char	xx2[4];
	u_char	longname[12];
	u_char	xx3[16];
	u_char	speed[4];
	u_char	xx4[12];
};

struct mvmeprom_time {
        u_char	year_BCD;
        u_char	month_BCD;
        u_char	day_BCD;
        u_char	wday_BCD;
        u_char	hour_BCD;
        u_char	min_BCD;
        u_char	sec_BCD;
        u_char	cal_BCD;
};

struct mvmeprom_dskio {
	u_char	ctrl_lun;
	u_char	dev_lun;
	u_short	status;
	void	*pbuffer;
	u_long	blk_num;
	u_short	blk_cnt;
	u_char	flag;
#define BUG_FILE_MARK	0x80
#define IGNORE_FILENUM	0x02
#define END_OF_FILE	0x01
	u_char	addr_mod;
};
#define MVMEPROM_BLOCK_SIZE	256

struct mvmeprom_args {
        u_int	dev_lun;
        u_int	ctrl_lun;
        u_int	flags;
        u_int	ctrl_addr;
        u_int	entry;
        u_int	conf_blk;
	char	*nbarg_start;
	char	*nbarg_end;
        char	*arg_start;
        char	*arg_end;
	u_int	cputyp;
};

#endif

#define MVMEPROM_CALL(x) \
	__asm volatile ("trap #15; .short " __STRING(x))
#define MVMEPROM_NOARG() \
	__asm volatile ("clrl %sp@-")
#define MVMEPROM_ARG1(arg) \
	__asm volatile ("movel %0, %%sp@-"::"d" (arg))
#define MVMEPROM_ARG2(arg0, arg1)				\
	__asm volatile ("movel %0, %%sp@-;"			\
			"movel %1, %%sp@-;"			\
			:: "d" (arg0), "d" (arg1):)
#define MVMEPROM_GETRES(ret) \
	__asm volatile ("movel %%sp@+,%0": "=d" (ret):)
#define MVMEPROM_GETSR(ret) \
	__asm volatile ("movew %%sr,%0": "=d" (ret):)
#define MVMEPROM_RETURN(ret) \
	MVMEPROM_GETRES(ret); \
	return (ret);			/* return a value (int) */
/* return a byte, ret must be int */
#define MVMEPROM_RETURN_BYTE(ret) \
	MVMEPROM_GETRES(ret); \
	return(int)((((unsigned int)(ret)) >> 24) & 0xff);
#define MVMEPROM_STATRET(ret) \
	MVMEPROM_GETSR(ret); \
	return ((ret & 0x4) == 0);	/* return a 'status' in the Z flag */

#define MVMEPROM_REG_DEVLUN	%d0
#define MVMEPROM_REG_CTRLLUN	%d1
#define MVMEPROM_REG_FLAGS	%d4
#define MVMEPROM_REG_CTRLADDR	%a0
#define MVMEPROM_REG_ENTRY	%a1
#define MVMEPROM_REG_CONFBLK	%a2
#define MVMEPROM_REG_NBARGSTART	%a3
#define MVMEPROM_REG_NBARGEND	%a4
#define MVMEPROM_REG_ARGSTART	%a5
#define MVMEPROM_REG_ARGEND	%a6

#define MVMEPROM_ARGS_DEVLUN	0x00
#define MVMEPROM_ARGS_CTRLLUN	0x04
#define MVMEPROM_ARGS_FLAGS	0x08
#define MVMEPROM_ARGS_CTRLADDR	0x0c
#define MVMEPROM_ARGS_ENTRY	0x10
#define MVMEPROM_ARGS_CONFBLK	0x14
#define MVMEPROM_ARGS_NBARGSTART 0x18
#define MVMEPROM_ARGS_NBARGEND	0x1c
#define MVMEPROM_ARGS_ARGSTART	0x20
#define MVMEPROM_ARGS_ARGEND	0x24
#define MVMEPROM_ARGS_CPUTYP	0x28
#define MVMEPROM_ARGS_MAX	0x2c


#ifndef RB_NOSYM
#define RB_NOSYM 0x400
#endif
#ifndef RB_SBOOT
#define RB_SBOOT 0x800
#endif

