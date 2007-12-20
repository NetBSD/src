/*	$NetBSD: linux_ioctl.h,v 1.25 2007/12/20 23:02:54 dsl Exp $	*/

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
int linux_machdepioctl(struct lwp *, const struct linux_sys_ioctl_args *, register_t *);
int linux_ioctl_cdrom(struct lwp *, const struct linux_sys_ioctl_args *,
    register_t *);
int linux_ioctl_termios(struct lwp *, const struct linux_sys_ioctl_args *,
    register_t *);
int linux_ioctl_socket(struct lwp *, const struct linux_sys_ioctl_args *,
    register_t *);
int linux_ioctl_hdio(struct lwp *, const struct linux_sys_ioctl_args *,
    register_t *);
int linux_ioctl_fdio(struct lwp *, const struct linux_sys_ioctl_args *uap,
                 register_t *retval);
int linux_ioctl_blkio(struct lwp *, const struct linux_sys_ioctl_args *uap,
                 register_t *retval);
int linux_ioctl_sg(struct lwp *, const struct linux_sys_ioctl_args *uap,
                 register_t *retval);
int linux_ioctl_mtio(struct lwp *, const struct linux_sys_ioctl_args *uap, 
                 register_t *retval);
__END_DECLS
#endif	/* !_KERNEL */

#if defined(__i386__)
#include <compat/linux/arch/i386/linux_ioctl.h>
#elif defined(__m68k__)
#include <compat/linux/arch/m68k/linux_ioctl.h>
#elif defined(__alpha__)
#include <compat/linux/arch/alpha/linux_ioctl.h>
#elif defined(__powerpc__)
#include <compat/linux/arch/powerpc/linux_ioctl.h>
#elif defined(__mips__)
#include <compat/linux/arch/mips/linux_ioctl.h>
#elif defined(__arm__)
#include <compat/linux/arch/arm/linux_ioctl.h>
#elif defined(__amd64__)
#include <compat/linux/arch/amd64/linux_ioctl.h>
#else
#error Undefined linux_ioctl.h machine type.
#endif

#define	_LINUX_IOC_NRMASK	((1 << _LINUX_IOC_NRBITS) - 1)
#define	_LINUX_IOC_TYPEMASK	((1 << _LINUX_IOC_TYPEBITS) - 1)
#define	_LINUX_IOC_SIZEMAEK	((1 << _LINUX_IOC_SIZEBITS) - 1)
#define _LINUX_IOC_DIRMASK	((1 << _LINUX_IOC_DIRBITS) - 1)

#define	_LINUX_IOC_TYPESHIFT	(_LINUX_IOC_NRSHIFT + _LINUX_IOC_NRBITS)
#define	_LINUX_IOC_SIZESHIFT	(_LINUX_IOC_TYPESHIFT + _LINUX_IOC_TYPEBITS)
#define	_LINUX_IOC_DIRSHIFT	(_LINUX_IOC_SIZESHIFT + _LINUX_IOC_SIZEBITS)

#define	_LINUX_IOC(dir,type,nr,size)		\
	(((nr)   << _LINUX_IOC_NRSHIFT) |	\
	 ((type) << _LINUX_IOC_TYPESHIFT) |	\
	 ((size) << _LINUX_IOC_SIZESHIFT) |	\
	 ((dir)  << _LINUX_IOC_DIRSHIFT))

#define _LINUX_IO(type,nr)		\
	_LINUX_IOC(_LINUX_IOC_NONE,(type),(nr),0)
#define	_LINUX_IOR(type,nr,size)	\
	_LINUX_IOC(_LINUX_IOC_READ,(type),(nr),sizeof(size))
#define	_LINUX_IOW(type,nr,size)	\
	_LINUX_IOC(_LINUX_IOC_WRITE,(type),(nr),sizeof(size))
#define	_LINUX_IOWR(type,nr,size)	\
	_LINUX_IOC(_LINUX_IOC_READ|_LINUX_IOC_WRITE,(type),(nr),sizeof(size))

#define _LINUX_IOC_DIR(nr)	\
	(((nr) >> _LINUX_IOC_DIRSHIFT) & _LINUX_IOC_DIRMASK)
#define _LINUX_IOC_TYPE(nr)	\
	(((nr) >> _LINUX_IOC_TYPESHIFT) & _LINUX_IOC_TYPEMASK)
#define _LINUX_IOC_NR(nr)	\
	(((nr) >> _LINUX_IOC_NRSHIFT) & _LINUX_IOC_NRMASK)
#define _LINUX_IOC_SIZE(nr)	\
	(((nr) >> _LINUX_IOC_SIZESHIFT) & _LINUX_IOC_SIZEMASK)

#define LINUX_IOCGROUP(x)	_LINUX_IOC_TYPE(x)

#endif /* !_LINUX_IOCTL_H */
