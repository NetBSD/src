/*	$NetBSD: vmem.h,v 1.6 2007/06/17 13:34:42 yamt Exp $	*/

/*-
 * Copyright (c)2006 YAMAMOTO Takashi,
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
 */

#ifndef _SYS_VMEM_H_
#define	_SYS_VMEM_H_

#include <sys/types.h>

#if !defined(_KERNEL)
#include <stdbool.h>
#endif /* !defined(_KERNEL) */

typedef struct vmem vmem_t;

typedef unsigned int vm_flag_t;

typedef	uintptr_t vmem_addr_t;
typedef size_t vmem_size_t;
#define	VMEM_ADDR_NULL	0

vmem_t *vmem_create(const char *, vmem_addr_t, vmem_size_t, vmem_size_t,
    vmem_addr_t (*)(vmem_t *, vmem_size_t, vmem_size_t *, vm_flag_t),
    void (*)(vmem_t *, vmem_addr_t, vmem_size_t), vmem_t *, vmem_size_t,
    vm_flag_t);
void vmem_destroy(vmem_t *);
vmem_addr_t vmem_alloc(vmem_t *, vmem_size_t, vm_flag_t);
void vmem_free(vmem_t *, vmem_addr_t, vmem_size_t);
vmem_addr_t vmem_xalloc(vmem_t *, vmem_size_t, vmem_size_t, vmem_size_t,
    vmem_size_t, vmem_addr_t, vmem_addr_t, vm_flag_t);
void vmem_xfree(vmem_t *, vmem_addr_t, vmem_size_t);
vmem_addr_t vmem_add(vmem_t *, vmem_addr_t, vmem_size_t, vm_flag_t);
vmem_size_t vmem_roundup_size(vmem_t *, vmem_size_t);
bool vmem_reap(vmem_t *);
void vmem_rehash_start(void);

/* vm_flag_t */
#define	VM_SLEEP	0x00000001
#define	VM_NOSLEEP	0x00000002
#define	VM_INSTANTFIT	0x00001000
#define	VM_BESTFIT	0x00002000

#endif /* !_SYS_VMEM_H_ */
