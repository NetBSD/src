/*	$NetBSD: linux_ioctl.h,v 1.11 1998/12/15 19:31:39 itohy Exp $	*/

/*-
 * Copyright (c) 1995, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden and Eric Haszlakiewicz.
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

#ifndef _LINUX_IOCTL_H
#define _LINUX_IOCTL_H

struct linux_sys_ioctl_args;

#ifdef _KERNEL
__BEGIN_DECLS
int linux_machdepioctl __P((struct proc *, void *, register_t *));
int linux_ioctl_cdrom __P((struct proc *, struct linux_sys_ioctl_args *,
    register_t *));
int linux_ioctl_termios __P((struct proc *, struct linux_sys_ioctl_args *,
    register_t *));
int linux_ioctl_socket __P((struct proc *, struct linux_sys_ioctl_args *,
    register_t *));
__END_DECLS
#endif	/* !_KERNEL */

#if defined(__i386__)
#include <compat/linux/arch/i386/linux_ioctl.h>
#elif defined(__m68k__)
#include <compat/linux/arch/m68k/linux_ioctl.h>
#elif defined(__alpha__)
#include <compat/linux/arch/alpha/linux_ioctl.h>
#else
#error Undefined linux_ioctl.h machine type.
#endif

#endif /* !_LINUX_IOCTL_H */
