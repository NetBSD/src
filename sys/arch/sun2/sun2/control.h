/*	$NetBSD: control.h,v 1.2.2.2 2001/04/21 17:54:55 bouyer Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass and Matthew Fredette.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * Macros for Sun2 control space
 */

/*
 * On the Sun3, the 32 bytes of IDPROM are at consecutive addresses in
 * control space starting at 0x00000000.  On the Sun2, the 32 bytes of
 * IDPROM are at offset 8 in each of the the first 32 pages of control
 * space, i.e., on the Sun2, in control space the page frame is used
 * as an index, and the page offset bits are used to select something
 * to use that index with.  I'm not sure which I like better.
 */

#define PGMAP_BASE       0x00000000
#define SEGMAP_BASE      0x00000005
#define SCONTEXT_REG     0x00000006
#define CONTEXT_REG      0x00000007
#define IDPROM_BASE      0x00000008
#define DIAG_REG         0x0000000B
#define BUSERR_REG       0x0000000C
#define SYSTEM_ENAB      0x0000000E

#define CONTROL_ADDR_MASK 0xFFFFF800
#define CONTROL_ADDR_BUILD(as, va) \
((as) | ((va) & CONTROL_ADDR_MASK))

#define CONTEXT_NUM 0x8
#define CONTEXT_MASK 0x7

#if defined(_KERNEL) || defined(_STANDALONE)

/* ctrlsp.S */
int   get_control_byte __P((vm_offset_t));
void  set_control_byte __P((vm_offset_t, int));
u_int get_control_word __P((vm_offset_t));
void  set_control_word __P((vm_offset_t, u_int));

/* control.c */
int  get_context __P((void));
void set_context __P((int));

int  get_segmap __P((vm_offset_t));
void set_segmap __P((vm_offset_t, int));

#if 0
/* Moved to pte.h (now a common interface). */
int  get_pte __P((vm_offset_t));
void set_pte __P((vm_offset_t, int));
#endif

#endif	/* _KERNEL | _STANDALONE */
