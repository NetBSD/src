/* $NetBSD: hawk.h,v 1.1.10.2 2014/08/20 00:02:54 tls Exp $ */
/*
 * Copyright (c) 2013 Linu Cherian
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain this list of conditions
 *    and the following disclaimer.
 * 2. Redistributions in binary form must reproduce this list of conditions
 *    and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANY
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _EVBARM_HAWK_HAWK_H
#define _EVBARM_HAWK_HAWK_H


/* OMAPL138 Memory Map */
#define OMAPL138_MEM_BASE	0xC0000000
#define OMAPL138_IO_BASE	0x01D00000 /* for static 1MB mapping */

/*
 * Kernel VM space: 192MB at KERNEL_VM_BASE
 */
#define	KERNEL_VM_BASE		((KERNEL_BASE + 0x01800000) & ~(0x400000-1))
#define KERNEL_VM_SIZE		0x0C000000

/*
 * We devmap IO starting at KERNEL_VM_BASE + KERNEL_VM_SIZE
 */
#define	OMAPL1X_KERNEL_IO_VBASE		(KERNEL_VM_BASE + KERNEL_VM_SIZE)

#endif /* _EVBARM_HAWK_HAWK_H */
