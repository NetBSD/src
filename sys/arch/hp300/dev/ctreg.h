/*	$NetBSD: ctreg.h,v 1.10.170.1 2021/07/14 18:04:04 martin Exp $	*/

/*
 * Copyright (c) 1982, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)ctreg.h	8.1 (Berkeley) 6/10/93
 */

struct	ct_iocmd {
	char	unit;
	char	saddr;
	short	addr0;
	long	addr;
	char	nop2;
	char	slen;
	long	len;
	char	nop3;
	char	cmd;
};

struct	ct_rscmd {
	char	unit;
	char	cmd;
};

struct	ct_stat {
	char	c_vu;
	char	c_pend;
	short	c_ref;
	short	c_fef;
	short	c_aef;
	short	c_ief;
	short	c_blk0;
	long	c_blk;
	char	c_pad[4];
};

struct	ct_ssmcmd {
	char	unit;
	char	cmd;
	short	refm;
	short	fefm;
	short	aefm;
	short	iefm;
};

struct	ct_srcmd {
	char	unit;
	char	nop;
	char	cmd;
	char	param;
};

struct	ct_soptcmd {
	char	unit;
	char	nop;
	char	cmd;
	char	opt;
};

struct	ct_ulcmd {
	char	unit;
	char	cmd;
};

struct	ct_wfmcmd {
	char	unit;
	char	cmd;
};

struct	ct_clearcmd {
	char	unit;
	char	cmd;
};

#define	CT7946ID	0x220
#define CT9145ID	0x268
#define	CT9144ID	0x260
#define	CT9144		0
#define	CT7912PID	0x209
#define	CT7914PID	0x20B
#define	CT88140		1
#define	CT35401ID	0x270

/* convert bytes to 1k tape block and back */
#define CTBTOK(x)	((x) >> 10)
#define CTKTOB(x)	((x) << 10)

#define	CTCTLR		15

#define	REF_MASK	0x0
#define	FEF_MASK	0x0
#define	AEF_MASK	0x0
#define	IEF_MASK	0xF970

#define	FEF_PF		0x0002
#define	FEF_REXMT	0x0001
#define	AEF_EOF		0x0010
#define	AEF_EOV		0x0008

#define	C_READ		0x00
#define	C_WRITE		0x02
#define	C_CLEAR		0x08
#define	C_STATUS	0x0d
#define	C_SADDR		0x10
#define	C_SLEN		0x18
#define	C_SUNIT(x)	(0x20 | (x))
#define	C_SVOL(x)	(0x40 | (x))
#define	C_NOP		0x34
#define C_DESC		0x35
#define	C_SOPT		0x38
#define	C_SREL		0x3b
#define	C_SSM		0x3e
#define	C_WFM		0x49
#define	C_UNLOAD	0x4a
#define C_REL		0xc0

#define	C_CMD		0x05
#define	C_EXEC		0x0e
#define	C_QSTAT		0x10
#define	C_TCMD		0x12

#define C_CC            1
#define C_SKSPAR        2
#define	C_OPT		4
#define C_SPAR          4
#define C_IMRPT         8
