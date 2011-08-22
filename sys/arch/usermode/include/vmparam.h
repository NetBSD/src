/* $NetBSD: vmparam.h,v 1.4 2011/08/22 15:36:23 reinoud Exp $ */

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

#include </usr/include/machine/vmparam.h>
#include <machine/pmap.h>
#include "opt_memsize.h"

extern paddr_t kmem_k_start, kmem_k_end;
extern paddr_t kmem_data_start, kmem_data_end;
extern paddr_t kmem_ext_start, kmem_ext_end;
extern paddr_t kmem_user_start, kmem_user_end;

#undef VM_MIN_KERNEL_ADDRESS
#define VM_MIN_KERNEL_ADDRESS	kmem_k_start

#undef VM_MAX_KERNEL_ADDRESS
#define VM_MAX_KERNEL_ADDRESS 	kmem_ext_end

#undef VM_MIN_ADDRESS
#define VM_MIN_ADDRESS		kmem_k_start

#undef VM_MAX_ADDRESS
#define VM_MAX_ADDRESS		kmem_user_end

#undef VM_MAXUSER_ADDRESS
#define VM_MAXUSER_ADDRESS	kmem_user_end

#endif /* !_ARCH_USERMODE_INCLUDE_VMPARAM_H */
