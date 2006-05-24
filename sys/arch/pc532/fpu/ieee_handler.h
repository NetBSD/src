/*	$NetBSD: ieee_handler.h,v 1.8.8.1 2006/05/24 10:57:00 yamt Exp $	*/

/*
 * IEEE floating point support for NS32081 and NS32381 fpus.
 * Copyright (c) 1995 Ian Dall
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * IAN DALL ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" CONDITION.
 * IAN DALL DISCLAIMS ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 */
/*
 *	File:	ieee_handler.h
 *	Author:	Ian Dall
 *	Date:	November 1995
 *
 *	Prototypes for entry points. Customization for in kernel version
 *      or unix signal handler version.
 *
 * HISTORY
 * 02-Apr-96 Matthias Pfaller <leo@dachau.marco.de>)
 *	Add NetBSD kernel support.
 *
 * 14-Dec-95  Ian Dall (Ian.Dall@dsto.defence.gov.au)
 *	First release.
 *
 */
#ifndef _IEEE_HANDLER_H_
#define _IEEE_HANDLER_H_

#if defined(KERNEL) || defined(_KERNEL)
#ifdef MACH

#include <ns532/thread.h>
#include <mach/vm_param.h>
#include "mach_kdb.h"
#if MACH_KDB
#define DEBUG
#endif

typedef struct {
	struct ns532_saved_state *regs;
	struct ns532_fp_state *fps;
} state;

#define LREGBASE(s) ((vaddr_t) (s)->fps)
#define LREGOFFSET(n) offsetof(struct ns532_fp_state, regs.l.l ## n)
#define FREGBASE(s) ((vaddr_t) (s)->fps)
#define FREGOFFSET(n) offsetof(struct ns532_fp_state, regs.f.f ## n)
#define REGBASE(s) ((vaddr_t) (s)->regs)
#define REGOFFSET(n) offsetof(struct ns532_saved_state, r ## n)

#define FSR fps->fsr
#define FP regs->fp
#define SP regs->usp
#define SB regs->sb
#define PC regs->pc
#define PSR regs->psr

#elif defined(__NetBSD__)  /* MACH */
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/user.h>

typedef struct lwp state;

#define LREGBASE(s) ((vaddr_t) (s)->l_addr->u_pcb.pcb_freg)
#define LREGOFFSET(n) (n * sizeof(double))
#define FREGBASE(s) ((vaddr_t) (s)->l_addr->u_pcb.pcb_freg)
#define FREGOFFSET(n) ((n & ~1) * sizeof(double) + (n & 1) * sizeof(float))
#define REGBASE(s) ((vaddr_t) (s)->l_md.md_regs)
#define REGOFFSET(n) offsetof(struct reg, r_r ## n)

#define FSR l_addr->u_pcb.pcb_fsr
#define FP l_md.md_regs->r_fp
#define SP l_md.md_regs->r_sp
#define SB l_md.md_regs->r_sb
#define PC l_md.md_regs->r_pc
#define PSR l_md.md_regs->r_psr

#define PSR_N	PSL_N
#define PSR_Z	PSL_Z
#define PSR_L	PSL_L

#else  /* __NetBSD__ */
#error OS unsupported
#endif /* __NetBSD__ */
#else /* KERNEL || _KERNEL */
#include <signal.h>

struct fstate {
	unsigned int fsr;
	union {
		struct {
			double l0, l2, l4, l6, l1, l3, l5, l7;
		} l;
		struct {
			float f0, f1, f2, f3, f4, f5, f6, f7;
		} f;
	} regs;
};

struct ns532_combined_state {
	struct sigcontext *scp;
	struct fstate fs;
};

typedef struct ns532_combined_state state;

#define LREG(REG) fs.regs.l.l ## REG
#define LREGBASE(s) ((vaddr_t) &(s)->fs)
#define LREGOFFSET(n) offsetof(struct fstate, regs.l.l ## n)
#define FREGBASE(s) ((vaddr_t) &(s)->fs)
#define FREGOFFSET(n) offsetof(struct fstate, regs.f.f ## n)
#define REGBASE(s) ((vaddr_t) (s)->scp)
#define REGOFFSET(n) offsetof(struct sigcontext, sc_reg[n])
#define FSR fs.fsr
#define FP scp->sc_fp
#define SP scp->sc_sp
#define SB scp->sc_sb
#define PC scp->sc_pc
#define PSR scp->sc_ps

int ieee_sig(int, int, struct sigcontext *);

#ifdef __NetBSD__
#define PSR_N	PSL_N
#define PSR_Z	PSL_Z
#define PSR_L	PSL_L
#endif /* __NetBSD */

#endif /* KERNEL */

int ieee_handle_exception(state *);

#endif /* _IEEE_HANDLER_H_ */
