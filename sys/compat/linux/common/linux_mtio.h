/* $NetBSD: linux_mtio.h,v 1.2 2005/12/11 12:20:19 christos Exp $ */

/*
 * Copyright (c) 2005 Soren S. Jorvang.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _LINUX_MTIO_H
#define _LINUX_MTIO_H

#define LINUX_MTIOCTOP		_LINUX_IOW('m', 1, struct linux_mtop)
#define LINUX_MTIOCGET		_LINUX_IOR('m', 2, struct linux_mtget)
#define LINUX_MTIOCPOS		_LINUX_IOR('m', 3, struct linux_mtpos)

struct	linux_mtop {
	short	mt_op;
	int	mt_count;
};

#define LINUX_MTRESET		0
#define LINUX_MTFSF		1
#define LINUX_MTBSF		2
#define LINUX_MTFSR		3
#define LINUX_MTBSR		4
#define LINUX_MTWEOF		5
#define LINUX_MTREW		6
#define LINUX_MTOFFL		7
#define LINUX_MTNOP		8
#define LINUX_MTRETEN		9
#define LINUX_MTBSFM		10
#define LINUX_MTFSFM		11
#define LINUX_MTEOM		12
#define LINUX_MTERASE		13
#define LINUX_MTRAS1		14
#define LINUX_MTRAS2		15
#define LINUX_MTRAS3		16
#define LINUX_MTSETBLK		20
#define LINUX_MTSETDENSITY	21
#define LINUX_MTSEEK		22
#define LINUX_MTTELL		23
#define LINUX_MTSETDRVBUFFER	24
#define LINUX_MTFSS		25
#define LINUX_MTBSS		26
#define LINUX_MTWSM		27
#define LINUX_MTLOCK		28
#define LINUX_MTUNLOCK		29
#define LINUX_MTLOAD		30
#define LINUX_MTUNLOAD		31
#define LINUX_MTCOMPRESSION	32
#define LINUX_MTSETPART		33
#define LINUX_MTMKPART		34

struct	linux_mtget {
#define LINUX_MT_ISUNKNOWN	0x01
	long	mt_type;
	long	mt_resid;
	long	mt_dsreg;
	long	mt_gstat;
	long	mt_erreg;
	daddr_t mt_fileno;
	daddr_t mt_blkno;
};

struct linux_mtpos {
	long	mt_blkno;
};

#endif /* !_LINUX_MTIO_H */
