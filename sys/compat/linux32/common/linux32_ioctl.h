/*	$NetBSD: linux32_ioctl.h,v 1.1.54.2 2007/12/27 00:44:16 mjf Exp $ */

/*-
 * Copyright (c) 1995-2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden, Eric Haszlakiewicz, and Emmanuel Dreyfus.
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

#ifndef _LINUX32_IOCTL_H
#define _LINUX32_IOCTL_H

#if defined(__amd64__)
#include <compat/linux32/arch/amd64/linux32_ioctl.h>
#endif

#ifdef _KERNEL
__BEGIN_DECLS
struct linux32_sys_ioctl_args;
int linux32_ioctl_termios(struct lwp *, const struct linux32_sys_ioctl_args *,
    register_t *);
__END_DECLS
#endif 

#define	_LINUX32_IOC_NRMASK	((1 << _LINUX32_IOC_NRBITS) - 1)
#define	_LINUX32_IOC_TYPEMASK	((1 << _LINUX32_IOC_TYPEBITS) - 1)
#define	_LINUX32_IOC_SIZEMAEK	((1 << _LINUX32_IOC_SIZEBITS) - 1)
#define _LINUX32_IOC_DIRMASK	((1 << _LINUX32_IOC_DIRBITS) - 1)

#define	_LINUX32_IOC_TYPESHIFT	(_LINUX32_IOC_NRSHIFT + _LINUX32_IOC_NRBITS)
#define	_LINUX32_IOC_SIZESHIFT	(_LINUX32_IOC_TYPESHIFT + _LINUX32_IOC_TYPEBITS)
#define	_LINUX32_IOC_DIRSHIFT	(_LINUX32_IOC_SIZESHIFT + _LINUX32_IOC_SIZEBITS)

#define	_LINUX32_IOC(dir,type,nr,size)		\
	(((nr)   << _LINUX32_IOC_NRSHIFT) |	\
	 ((type) << _LINUX32_IOC_TYPESHIFT) |	\
	 ((size) << _LINUX32_IOC_SIZESHIFT) |	\
	 ((dir)  << _LINUX32_IOC_DIRSHIFT))

#define _LINUX32_IO(type,nr)		\
    _LINUX32_IOC(_LINUX32_IOC_NONE,(type),(nr),0)
#define	_LINUX32_IOR(type,nr,size)	\
    _LINUX32_IOC(_LINUX32_IOC_READ,(type),(nr),sizeof(size))
#define	_LINUX32_IOW(type,nr,size)	\
    _LINUX32_IOC(_LINUX32_IOC_WRITE,(type),(nr),sizeof(size))
#define	_LINUX32_IOWR(type,nr,size)	\
    _LINUX32_IOC(_LINUX32_IOC_READ|_LINUX32_IOC_WRITE,(type),(nr),sizeof(size))

#define _LINUX32_IOC_DIR(nr)	\
	(((nr) >> _LINUX32_IOC_DIRSHIFT) & _LINUX32_IOC_DIRMASK)
#define _LINUX32_IOC_TYPE(nr)	\
	(((nr) >> _LINUX32_IOC_TYPESHIFT) & _LINUX32_IOC_TYPEMASK)
#define _LINUX32_IOC_NR(nr)	\
	(((nr) >> _LINUX32_IOC_NRSHIFT) & _LINUX32_IOC_NRMASK)
#define _LINUX32_IOC_SIZE(nr)	\
	(((nr) >> _LINUX32_IOC_SIZESHIFT) & _LINUX32_IOC_SIZEMASK)

#define LINUX32_IOCGROUP(x)	_LINUX32_IOC_TYPE(x)

#endif /* !_LINUX32_IOCTL_H */
