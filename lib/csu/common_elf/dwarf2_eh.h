/*	$NetBSD: dwarf2_eh.h,v 1.1 2001/08/03 05:54:44 thorpej Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * Definitions for DWARF2 stack frame unwinding/exception handling.
 */

#ifdef __GNUC__
/*
 * We must pass a DWARF2 unwind object to __register_frame_info().
 * Since we don't reference any members of this object, but rather
 * only provide storage for it, we just declare it in a simple,
 * dumb way.  We need room for 6 pointers in GCC 2.95.3 and GCC 3.0,
 * but declare it with a little slop at the end.
 */
struct dwarf2_eh_object {
	void *space[8];
};

/*
 * These routines are provided by libgcc to register/unregister
 * frame info.  Note these prototypes must generate weak references
 * (even though the routines in libgcc have strong definitions).
 * This is so that we can link with a libgcc that doesn't have these
 * routines (e.g. one that uses sjlj exceptions).
 */
extern void __register_frame_info(void *,
    struct dwarf2_eh_object *) __attribute__((weak));
extern void __deregister_frame_info(void *) __attribute__((weak));
#endif /* __GNUC__ */
