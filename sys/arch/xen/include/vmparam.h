/*	$NetBSD: vmparam.h,v 1.1.22.3 2007/10/27 11:29:12 yamt Exp $	*/

/*-
 * Copyright (c)2005 YAMAMOTO Takashi,
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

#if !defined(_VMPARAM_H_)

/* XXX hack */
#define	PDIR_SLOT_PTE	PDSLOT_PTE
#define	PDIR_SLOT_APTE	PDSLOT_APTE
#define	PDIR_SLOT_KERN	PDSLOT_KERN
#define	NBPD		NBPD_L2
#define	PDSHIFT		L2_SHIFT
#define	PD_MASK		L2_MASK
#define	PT_MASK		L1_MASK
#define	PTES_PER_PTP	NPTEPG

#include <i386/vmparam.h>

#undef VM_PHYSSEG_MAX
#define	VM_PHYSSEG_MAX	1

#undef VM_NFREELIST
#undef VM_FREELIST_FIRST16
#define	VM_NFREELIST	1
#endif /* _VMPARAM_H_ */
