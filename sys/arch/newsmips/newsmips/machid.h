/*	$NetBSD: machid.h,v 1.6.166.1 2019/06/10 22:06:35 christos Exp $	*/
/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Sony Corp. and Kazumasa Utashiro of Software Research Associates, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 * from: $Hdr: machid.h,v 4.300 91/06/09 06:35:19 root Rel41 $ SONY
 *
 *	@(#)machid.h	8.1 (Berkeley) 6/11/93
 */

#ifndef __MACHID__
#define __MACHID__ 1

/*
 * machine id number definition.
 */
#define	ICK001	1
#define	ICK00X	2
#define	NWS799	3
#define	NWS800	4
#define	NWS801	5
#define	NWS802	6
#define	NWS711	7
#define	NWS721	8
#define	NWS1850	9
#define	NWS810	10
#define	NWS811	11
#define	NWS1830	12
#define	NWS1750	13
#define	NWS1720	14
#define	NWS1930	15
#define	NWS1960	16
#define	NWS712	17
#define	NWS1860	18
#define	PWS1630	19
#define	NWS820	20
#define	NWS821	21
#define	NWS1760	22
#define	NWS1710	23
#define	NWS830	30
#define	NWS831	31
#define	NWS841	41
#define	PWS1570	52
#define	PWS1590	54
#define	NWS1520	56
#define	PWS1550	73
#define	PWS1520	74
#define	PWS1560	75
#define	NWS1530	76
#define	NWS1580	77
#define	NWS1510	78
#define	NWS1410	81
#define	NWS1450	85
#define	NWS1460	86
#define	NWS891	91
#define	NWS911	111
#define	NWS921	121
#define	NWB235	235
#define	NWB235A	236
#define	NWXRES	255

#ifdef __mips__
#define	MACHID_MODEL(X)		(((X)>>18)&0x1f)
#define	MACHID_SERIAL(X)	((X)&0x3ffff)
#else /* __mips__ */
#define	MACHID_MODEL(X)		(((X)>>16)&0xff)
#define	MACHID_SERIAL(X)	((X)&0xffff)
#endif /* __mips__ */

#ifndef LOCORE

/*
 * MPU board id number definition.
 */
#define MPU0 0				/* not used */
#define MPU1 1				/* ICKI */
#define MPU2 2				/* 799/801/810/811/820/821 */
#define MPU3 3				/* 802/830/841/911/921 */
#define MPU4 4				/* 711 */
#define MPU5 5				/* 1830/1850/1860/1930/1960 */
#define MPU6 6				/* 712/721 */
#define MPU7 7				/* 14XX/15XX */
#define MPU8 8				/* 16XX/17XX */
#define MPU9 9				/*  */

struct machid {
/*00*/	u_short	m_pwb;			/* MPU board number */
/*02*/	u_short	m_model;		/* MPU dependent model code */
/*04*/	u_int	m_serial;		/* machine serial number */
/*08*/	u_int	m_reserve0;
/*0c*/	u_int	m_reserve1;
/*10*/
};

union omachid {
/*00*/	struct om_field {
		u_int	fi_reserve:2,
			fi_pwb:7,	/* MPU (printed wired) board number */
			fi_model:5,	/* MPU dependent model code */
			fi_serial:18;	/* machine serial number */
	} om_fi;
/*00*/	u_int	om_data;
/*04*/
};

struct machine_type {
/*00*/	int	m_model_id;
/*04*/	char	*m_model_name;
/*08*/	char	*m_machine_name;
/*0c*/	char	*m_maincpu;
/*10*/	char	*m_subcpu;
/*14*/	char	*m_fpa;
/*18*/	int	m_board_id;
/*1c*/	int	m_cachecntl;
/*20*/	int	m_maxusers;		/* not used */
/*24*/	int	m_dcachesize;		/* not used */
/*28*/	int	m_icachesize;		/* not used */
/*2c*/
};

#if defined(news700) || defined(news800) || defined(news1700) || defined(news1800)
struct idrom {
/*00*/	unsigned char	id_model;
/*01*/	unsigned char	id_serial[2];
/*03*/	unsigned char	id_lot;
/*04*/	unsigned char	id_reserved[2];
/*06*/	unsigned char	id_chksum0[2];
/*08*/	unsigned char	id_ether[6];
/*0e*/	unsigned char	id_chksum1[2];
/*10*/
};
#endif /* news700 || news800 || news1700 || news1800 */

#if defined(news1200) || defined(news3400) || defined(news3800) || defined(news5000) || defined(news4000)
struct idrom {
/*00*/	unsigned char	id_id;          	/* always 0xff */
/*01*/	unsigned char	id_netid[5];    	/* network ID */
/*06*/	unsigned short	id_csum1;       	/* checksum 1 */
/*08*/	unsigned char	id_macadrs[6];  	/* MAC (ethernet) address */
/*0e*/	unsigned short	id_csum2;       	/* checksum 2 */
/*10*/	unsigned short	id_boardid;     	/* CPU board ID */
/*12*/	unsigned short	id_modelid;     	/* model ID */
/*14*/	unsigned int	id_serial;      	/* serial number */
/*18*/	unsigned short	id_year;
/*1a*/	unsigned short	id_month;
/*1c*/	unsigned char	id_zone[4];
/*20*/	char		id_board[16];
/*30*/	char		id_model[16];
/*40*/	char		id_machine[16];
/*50*/	char		id_cpu[16];
/*60*/	char		id_iop[16];
/*70*/	unsigned char	id_reserved[12];
/*7c*/	unsigned int	id_csum3;
/*80*/
};
#endif /* news1200 || news3400 || news3800 || news5000 || news4000 */

#endif /* !LOCORE */

#endif /* __MACHID__ */
