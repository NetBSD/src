/*	$NetBSD: md_var.h,v 1.1.142.1 2017/08/28 17:51:42 skrll Exp $	*/

/*-
 * Copyright (c) 1998 Doug Rabson
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
 *
 * $FreeBSD$
 */

#ifndef _IA64_MD_VAR_H_
#define	_IA64_MD_VAR_H_

/*
 * Miscellaneous machine-dependent declarations.
 */

struct ia64_fdesc {
	uint64_t	func;
	uint64_t	gp;
};

#define FDESC_FUNC(fn)  (((struct ia64_fdesc *) fn)->func)
#define FDESC_GP(fn)    (((struct ia64_fdesc *) fn)->gp)

/* Convenience macros to decompose CFM & ar.pfs. */
#define	IA64_CFM_SOF(x)		((x) & 0x7f)
#define	IA64_CFM_SOL(x)		(((x) >> 7) & 0x7f)
#define	IA64_CFM_SOR(x)		(((x) >> 14) & 0x0f)
#define	IA64_CFM_RRB_GR(x)	(((x) >> 18) & 0x7f)
#define	IA64_CFM_RRB_FR(x)	(((x) >> 25) & 0x7f)
#define	IA64_CFM_RRB_PR(x)	(((x) >> 32) & 0x3f)

#define IA64_RNATINDEX(x)	(((x) & 0x1f8) >> 3)

#ifdef _KERNEL

extern	char	sigcode[];
extern	char	esigcode[];
extern	int	szsigcode;
extern	long	Maxmem;
extern	vaddr_t kernstart, kernend;


struct _special;
struct fpreg;
struct reg;
struct thread;
struct trapframe;

void	busdma_swi(void);
int	copyout_regstack(struct thread *, uint64_t *, uint64_t *);
void	cpu_mp_add(u_int, u_int, u_int);
int	do_ast(struct trapframe *);
void	ia32_trap(int, struct trapframe *);
int	ia64_count_cpus(void);
void	ia64_flush_dirty(struct thread *, struct _special *);
uint64_t ia64_get_hcdp(void);
int	ia64_highfp_drop(struct thread *);
int	ia64_highfp_save(struct thread *);
void	ia64_init(void);
void	ia64_probe_sapics(void);
int	interrupt(uint64_t, struct trapframe *);
void	map_gateway_page(void);
void	map_pal_code(void);
void	map_vhpt(uintptr_t);
void	os_boot_rendez(void);
void	os_mca(void);
int	syscall(struct trapframe *);
void	trap(int, struct trapframe *);
void	trap_panic(int, struct trapframe *);
int	unaligned_fixup(struct trapframe *, struct thread *);
void	ia64_sync_icache(vaddr_t, vaddr_t);
#endif	/* _KERNEL */

#endif /* !_IA64_MD_VAR_H_ */
