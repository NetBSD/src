/*	$NetBSD: highmem.h,v 1.5 2018/08/27 15:13:17 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef _LINUX_HIGHMEM_H_
#define _LINUX_HIGHMEM_H_

#include <sys/types.h>
#include <sys/queue.h>

#include <uvm/uvm_extern.h>

#include <linux/kernel.h>
#include <linux/mm_types.h>

/* XXX Make the nm output a little more greppable...  */
#define	kmap(p)			linux_kmap(p)
#define	kmap_atomic(p)		linux_kmap_atomic(p)
#define	kunmap(p)		linux_kunmap(p)
#define	kunmap_atomic(p)	linux_kunmap_atomic(p)

/* XXX Kludge!  */
#define	kmap_atomic_prot(page, prot)	kmap_atomic(page)

int	linux_kmap_init(void);
void	linux_kmap_fini(void);

void *	kmap_atomic(struct page *);
void	kunmap_atomic(void *);

void *	kmap(struct page *);
void	kunmap(struct page *);

#endif  /* _LINUX_HIGHMEM_H_ */
