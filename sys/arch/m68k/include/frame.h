/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	from: Utah Hdr: frame.h 1.1 90/07/09
 *	from: @(#)frame.h	7.2 (Berkeley) 11/2/90
 *	$Id: frame.h,v 1.4 1994/01/26 21:35:37 mw Exp $
 */

struct frame {
	int	f_regs[16];
	short	f_stackadj;
	u_short	f_sr;
	u_int	f_pc;
	u_short	f_format:4,
		f_vector:12;
	union F_u {
		struct fmt2 {
			u_int	f_iaddr;
		} F_fmt2;

		struct fmt3 {
			u_int	f_iaddr;
		} F_fmt3;

		struct fmt7 {
			u_int	f_ea;
			u_short	f_ssw;
			u_short	f_wb3s;
			u_short	f_wb2s;
			u_short	f_wb1s;
			u_int	f_fa;
			u_int	f_wb3a;
			u_int	f_wb3d;
			u_int	f_wb2a;
			u_int	f_wb2d;
			u_int	f_wb1a;
			u_int	f_wb1d;
			u_int	f_pd1;
			u_int	f_pd2;
			u_int	f_pd3;
		} F_fmt7;

		struct fmt9 {
			u_int	f_iaddr;
			u_short	f_iregs[4];
		} F_fmt9;

		struct fmtA {
			u_short	f_ir0;
			u_short	f_ssw;
			u_short	f_ipsc;
			u_short	f_ipsb;
			u_int	f_dcfa;
			u_short	f_ir1, f_ir2;
			u_int	f_dob;
			u_short	f_ir3, f_ir4;
		} F_fmtA;

		struct fmtB {
			u_short	f_ir0;
			u_short	f_ssw;
			u_short	f_ipsc;
			u_short	f_ipsb;
			u_int	f_dcfa;
			u_short	f_ir1, f_ir2;
			u_int	f_dob;
			u_short	f_ir3, f_ir4;
			u_short	f_ir5, f_ir6;
			u_int	f_sba;
			u_short	f_ir7, f_ir8;
			u_int	f_dib;
			u_short	f_iregs[22];
		} F_fmtB;
	} F_u;
};

#define	f_fmt2		F_u.F_fmt2
#define f_fmt3		F_u.F_fmt3
#define	f_fmt7		F_u.F_fmt7
#define	f_fmtA		F_u.F_fmtA
#define	f_fmtB		F_u.F_fmtB

/* common frame size */
#define	CFSIZE		(sizeof(struct frame) - sizeof(union F_u))
#define	NFMTSIZE	8

#define	FMT0		0x0
#define	FMT1		0x1
#define	FMT2		0x2
#define	FMT3		0x3
#define	FMT7		0x7
#define	FMT9		0x9
#define	FMTA		0xA
#define	FMTB		0xB

/* frame specific info sizes */
#define	FMT0SIZE	0
#define	FMT1SIZE	0
#define	FMT2SIZE	sizeof(struct fmt2)
#define	FMT3SIZE	sizeof(struct fmt3)
#define	FMT7SIZE	sizeof(struct fmt7)
#define	FMT9SIZE	sizeof(struct fmt9)
#define	FMTASIZE	sizeof(struct fmtA)
#define	FMTBSIZE	sizeof(struct fmtB)

#define	V_BUSERR	0x008
#define	V_ADDRERR	0x00C
#define	V_TRAP1		0x084

/* 68040 fault frame */
#define SSW_CP		0x8000		/* Continuation - Floating-Point Post */
#define SSW_CU		0x4000		/* Continuation - Unimpl. FP */
#define SSW_CT		0x2000		/* Continuation - Trace */
#define SSW_CM		0x1000		/* Continuation - MOVEM */
#define SSW_MA		0x0800		/* Misaligned access */
#define SSW_ATC		0x0400		/* ATC fault */
#define SSW_LK		0x0200		/* Locked transfer */
#define SSW_RW040	0x0100		/* Read/Write */
#define SSW_SZMASK	0x0060		/* Transfer size */
#define SSW_TTMASK	0x0018		/* Transfer type */
#define SSW_TMMASK	0x0007		/* Transfer modifier */

#define WBS_TMMASK	0x0007
#define WBS_TTMASK	0x0018
#define WBS_SZMASK	0x0060
#define WBS_VALID	0x0080

#define WBS_SIZE_BYTE	0x0020
#define WBS_SIZE_WORD	0x0040
#define WBS_SIZE_LONG	0x0000
#define WBS_SIZE_LINE	0x0060

#define WBS_TT_NORMAL	0x0000
#define WBS_TT_MOVE16	0x0008
#define WBS_TT_ALTFC	0x0010
#define WBS_TT_ACK	0x0018

#define WBS_TM_PUSH	0x0000
#define WBS_TM_UDATA	0x0001
#define WBS_TM_UCODE	0x0002
#define WBS_TM_MMUTD	0x0003
#define WBS_TM_MMUTC	0x0004
#define WBS_TM_SDATA	0x0005
#define WBS_TM_SCODE	0x0006
#define WBS_TM_RESV	0x0007

#define	MMUSR_PA_MASK	0xfffff000
#define MMUSR_B		0x00000800
#define MMUSR_G		0x00000400
#define MMUSR_U1	0x00000200
#define MMUSR_U0	0x00000100
#define MMUSR_S		0x00000080
#define MMUSR_CM	0x00000060
#define MMUSR_M		0x00000010
#define MMUSR_0		0x00000008
#define MMUSR_W		0x00000004
#define MMUSR_T		0x00000002
#define MMUSR_R		0x00000001

/* 68020/68030 fault frame */
#define	SSW_RC		0x2000
#define	SSW_RB		0x1000
#define	SSW_DF		0x0100
#define	SSW_RM		0x0080
#define	SSW_RW		0x0040
#define	SSW_FCMASK	0x0007

struct fpframe {
	union FPF_u1 {
		u_int	FPF_null;
		struct {
			u_char	FPF_version;
			u_char	FPF_fsize;
			u_short	FPF_res1;
		} FPF_nonnull;
	} FPF_u1;
	union FPF_u2 {
		struct fpidle {
			u_short	fpf_ccr;
			u_short	fpf_res2;
			u_int	fpf_iregs1[8];
			u_int	fpf_xops[3];
			u_int	fpf_opreg;
			u_int	fpf_biu;
		} FPF_idle;

		struct fpbusy {
			u_int	fpf_iregs[53];
		} FPF_busy;

	} FPF_u2;
	u_int	fpf_regs[8*3];
	u_int	fpf_fpcr;
	u_int	fpf_fpsr;
	u_int	fpf_fpiar;
};

#define fpf_null	FPF_u1.FPF_null
#define fpf_version	FPF_u1.FPF_nonnull.FPF_version
#define fpf_fsize	FPF_u1.FPF_nonnull.FPF_fsize
#define fpf_res1	FPF_u1.FPF_nonnull.FPF_res1
#define fpf_idle	FPF_u2.FPF_idle
#define fpf_busy	FPF_u2.FPF_busy
