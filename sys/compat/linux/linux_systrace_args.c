/*	$NetBSD: linux_systrace_args.c,v 1.1.18.2 2017/12/03 11:36:53 jdolecek Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Eric Haszlakiewicz.
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

/* XXX XXX This exists to keep kdump and friends happy. */

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: linux_systrace_args.c,v 1.1.18.2 2017/12/03 11:36:53 jdolecek Exp $");

#if defined(__i386__)
#include "../../sys/compat/linux/arch/i386/linux_systrace_args.c"
#elif defined(__m68k__)
#include "../../sys/compat/linux/arch/m68k/linux_systrace_args.c"
#elif defined(__alpha__)
#include "../../sys/compat/linux/arch/alpha/linux_systrace_args.c"
#elif defined(__powerpc__)
#include "../../sys/compat/linux/arch/powerpc/linux_systrace_args.c"
#elif defined(__mips__)
#include "../../sys/compat/linux/arch/mips/linux_systrace_args.c"
#elif defined(__arm__)
#include "../../sys/compat/linux/arch/arm/linux_systrace_args.c"
#elif defined(__amd64__)
#include "../../sys/compat/linux/arch/amd64/linux_systrace_args.c"
#else
#error "fix me"
#endif
