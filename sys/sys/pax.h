/* $NetBSD: pax.h,v 1.29 2023/11/22 12:15:09 martin Exp $ */

/*-
 * Copyright (c) 2006 Elad Efrat <elad@NetBSD.org>
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#ifndef _SYS_PAX_H_
#define _SYS_PAX_H_

#include <uvm/uvm_extern.h>

#define P_PAX_ASLR	0x01	/* Enable ASLR */
#define P_PAX_MPROTECT	0x02	/* Enable Mprotect */
#define P_PAX_GUARD	0x04	/* Enable Segvguard */

struct lwp;
struct proc;
struct exec_package;
struct vmspace;

#ifdef PAX_ASLR
/*
 * We stick this here because we need it in kern/exec_elf.c for now.
 */
#ifndef PAX_ASLR_DELTA_EXEC_LEN
#define	PAX_ASLR_DELTA_EXEC_LEN	12
#endif
#endif /* PAX_ASLR */
#ifdef PAX_ASLR_DEBUG
extern int pax_aslr_debug;
#endif

#if defined(PAX_MPROTECT) || defined(PAX_SEGVGUARD) || defined(PAX_ASLR)
void pax_init(void);
void pax_set_flags(struct exec_package *, struct proc *);
void pax_setup_elf_flags(struct exec_package *, uint32_t);
#else
static inline void
pax_init(void)
{
}
static inline void
pax_set_flags(struct exec_package *epp, struct proc *p)
{
}
static inline void
pax_setup_elf_flags(struct exec_package *epp, uint32_t flags)
{
}
#endif

#ifdef PAX_MPROTECT

vm_prot_t pax_mprotect_maxprotect(
# ifdef PAX_MPROTECT_DEBUG
    const char *, size_t,
# endif
    struct lwp *, vm_prot_t, vm_prot_t, vm_prot_t);
int pax_mprotect_validate(
# ifdef PAX_MPROTECT_DEBUG
    const char *, size_t,
# endif
    struct lwp *, vm_prot_t);
int pax_mprotect_prot(struct lwp *);

#else

static inline vm_prot_t
pax_mprotect_maxprotect(struct lwp *l, vm_prot_t prot, vm_prot_t extra,
    vm_prot_t max)
{
	return max;
}
static inline vm_prot_t
pax_mprotect_validate(struct lwp *l, vm_prot_t prot)
{
	return 0;
}
static inline int
pax_mprotect_prot(struct lwp *l)
{
	return 0;
}

#endif

#if defined(PAX_MPROTECT) && defined(PAX_MPROTECT_DEBUG)
# define PAX_MPROTECT_MAXPROTECT(l, active, extra, max) \
   pax_mprotect_maxprotect(__FILE__, __LINE__, (l), (active), (extra), (max))
# define PAX_MPROTECT_VALIDATE(l, prot) \
   pax_mprotect_validate(__FILE__, __LINE__, (l), (prot))
#else
# define PAX_MPROTECT_MAXPROTECT(l, active, extra, max) \
   pax_mprotect_maxprotect((l), (active), (extra), (max))
# define PAX_MPROTECT_VALIDATE(l, prot) \
   pax_mprotect_validate((l), (prot))
#endif

#ifdef PAX_SEGVGUARD
int pax_segvguard(struct lwp *, struct vnode *, const char *, bool);
void pax_segvguard_cleanup(struct vnode *);
#endif

#ifdef PAX_ASLR
#define	PAX_ASLR_DELTA(delta, lsb, len)	\
    (((delta) & ((1UL << (len)) - 1)) << (lsb))
void pax_aslr_init_vm(struct lwp *, struct vmspace *, struct exec_package *);
void pax_aslr_stack(struct exec_package *, vsize_t *);
uint32_t pax_aslr_stack_gap(struct exec_package *);
vaddr_t pax_aslr_exec_offset(struct exec_package *, vaddr_t);
voff_t pax_aslr_rtld_offset(struct exec_package *, vaddr_t, int);
void pax_aslr_mmap(struct lwp *, vaddr_t *, vaddr_t, int);
#else
static inline void
pax_aslr_init_vm(struct lwp *l, struct vmspace *vm, struct exec_package *epp)
{
}
static inline void
pax_aslr_stack(struct exec_package *epp, vsize_t *max_stack_size)
{
}
static inline uint32_t
pax_aslr_stack_gap(struct exec_package *epp)
{
	return 0;
}
static inline vaddr_t
pax_aslr_exec_offset(struct exec_package *epp, vaddr_t align)
{
	return MAX(align, (vaddr_t)PAGE_SIZE);
}
static inline voff_t
pax_aslr_rtld_offset(struct exec_package *epp, vaddr_t align, int use_topdown)
{
	return 0;
}
static inline void
pax_aslr_mmap(struct lwp *l, vaddr_t *addr, vaddr_t orig_addr, int flags)
{
}
#endif

#endif /* !_SYS_PAX_H_ */
