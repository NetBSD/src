/* $NetBSD: vmparam.h,v 1.16.6.1 2014/08/20 00:03:27 tls Exp $ */

/*-
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
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

#ifndef _ARCH_USERMODE_INCLUDE_VMPARAM_H
#define _ARCH_USERMODE_INCLUDE_VMPARAM_H

#include <machine/pmap.h>
#include "opt_memsize.h"

#define __USE_TOPDOWN_VM

extern paddr_t kmem_k_start, kmem_k_end;
extern paddr_t kmem_kvm_start, kmem_kvm_end;
extern paddr_t kmem_kvm_cur_end;
extern paddr_t kmem_user_start, kmem_user_end;

#define VM_MIN_ADDRESS		kmem_user_start
#define VM_MAX_ADDRESS		kmem_user_end
#define VM_MAXUSER_ADDRESS	kmem_user_end
#define VM_MIN_KERNEL_ADDRESS	kmem_kvm_start
#define VM_MAX_KERNEL_ADDRESS 	kmem_kvm_end

#define VM_PHYSSEG_STRAT	VM_PSTRAT_BIGFIRST
#define VM_PHYSSEG_MAX		1
#define	VM_NFREELIST		1
#define	VM_FREELIST_DEFAULT	0

#define	USRSTACK		VM_MAXUSER_ADDRESS

/*
 * When an architecture has little KVA then override the default pager_map
 * size in its block by limiting it like this:
 *
 * #define PAGER_MAP_DEFAULT_SIZE	(8 * 1024 * 1024)
 */

#if defined(__i386__) 
#define	PAGE_SHIFT		12
#define	PAGE_SIZE		(1 << PAGE_SHIFT)
#define	PAGE_MASK		(PAGE_SIZE - 1)
#define	MAXSSIZ			(64 * 1024 * 1024)
#define	MAXTSIZ			(64 * 1024 * 1024)
#define	MAXDSIZ			(3U * 1024 * 1024 * 1024)
#define DFLSSIZ			(2 * 1024 * 1024)
#define	DFLDSIZ			(256 * 1024 * 1024)
#elif defined(__x86_64__)
#define	PAGE_SHIFT		12
#define	PAGE_SIZE		(1 << PAGE_SHIFT)
#define	PAGE_MASK		(PAGE_SIZE - 1)
#define	MAXSSIZ			(128 * 1024 * 1024)
#define	MAXTSIZ			(64 * 1024 * 1024)
#define	MAXDSIZ			(8L * 1024 * 1024 * 1024)
#define DFLSSIZ			(4 * 1024 * 1024)
#define	DFLDSIZ			(256 * 1024 * 1024)
#elif defined(__arm__)
#define	PAGE_SHIFT		12
#define	PAGE_SIZE		(1 << PAGE_SHIFT)
#define	PAGE_MASK		(PAGE_SIZE - 1)
#define	MAXSSIZ			(64 * 1024 * 1024)
#define	MAXTSIZ			(64 * 1024 * 1024)
#define	MAXDSIZ			(3U * 1024 * 1024 * 1024)
#define DFLSSIZ			(2 * 1024 * 1024)
#define	DFLDSIZ			(256 * 1024 * 1024)
#else
#error "platform not supported"
#endif

#endif /* !_ARCH_USERMODE_INCLUDE_VMPARAM_H */
