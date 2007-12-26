/*	$NetBSD: freebsd_ioctl.c,v 1.13.16.1 2007/12/26 19:48:53 ad Exp $	*/

/*
 * Copyright (c) 1995 Frank van der Linden
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project
 *      by Frank van der Linden
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: freebsd_ioctl.c,v 1.13.16.1 2007/12/26 19:48:53 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/sockio.h>

#include <sys/syscallargs.h>

#include <net/if.h>

#include <compat/sys/sockio.h>

#include <compat/freebsd/freebsd_syscallargs.h>
#include <compat/common/compat_util.h>
#include <compat/freebsd/freebsd_ioctl.h>

#include <compat/ossaudio/ossaudio.h>
#include <compat/ossaudio/ossaudiovar.h>

/* The FreeBSD and OSS(Linux) encodings of ioctl R/W differ. */
static void freebsd_to_oss(const struct freebsd_sys_ioctl_args *,
			   struct oss_sys_ioctl_args *);

static void
freebsd_to_oss(const struct freebsd_sys_ioctl_args *uap, struct oss_sys_ioctl_args *rap)
{
	u_long ocmd, ncmd;

        ocmd = SCARG(uap, com);
        ncmd = ocmd &~ FREEBSD_IOC_DIRMASK;
	switch(ocmd & FREEBSD_IOC_DIRMASK) {
        case FREEBSD_IOC_VOID:  ncmd |= OSS_IOC_VOID;  break;
        case FREEBSD_IOC_OUT:   ncmd |= OSS_IOC_OUT;   break;
        case FREEBSD_IOC_IN:    ncmd |= OSS_IOC_IN;    break;
        case FREEBSD_IOC_INOUT: ncmd |= OSS_IOC_INOUT; break;
        }
        SCARG(rap, fd) = SCARG(uap, fd);
        SCARG(rap, com) = ncmd;
        SCARG(rap, data) = SCARG(uap, data);
}


static void freebsd_to_netbsd_ifioctl(const struct freebsd_sys_ioctl_args *uap,
				      struct sys_ioctl_args *nap);

static void
freebsd_to_netbsd_ifioctl(const struct freebsd_sys_ioctl_args *uap, struct sys_ioctl_args *nap)
{
	u_long ocmd, ncmd;
	ocmd = SCARG(uap, com);
	switch (ocmd) {
	case FREEBSD_SIOCALIFADDR:
		ncmd =SIOCALIFADDR;
		break;
	case FREEBSD_SIOCGLIFADDR:
		ncmd =SIOCGLIFADDR;
		break;
	case FREEBSD_SIOCDLIFADDR:
		ncmd =SIOCDLIFADDR;
		break;
	case FREEBSD_SIOCGIFMTU:
		ncmd = SIOCGIFMTU;
		break;
	case FREEBSD_SIOCSIFMTU:
		ncmd = SIOCSIFMTU;
		break;
	default:
		ncmd = ocmd;
		break;
	}
	SCARG(nap, fd) = SCARG(uap, fd);
	SCARG(nap, com) = ncmd;
	SCARG(nap, data) = SCARG(uap, data);
}

int
freebsd_sys_ioctl(struct lwp *l, const struct freebsd_sys_ioctl_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(u_long) com;
		syscallarg(void *) data;
	} */
        struct oss_sys_ioctl_args ap;
	struct sys_ioctl_args nap;

	/*
	 * XXX - <sys/cdio.h>'s incompatibility
	 *	_IO('c', 25..27, *):	incompatible
	 *	_IO('c', 28... , *):	not exist
	 */
	/* XXX - <sys/mtio.h> */
	/* XXX - <sys/scsiio.h> */
	/* XXX - should convert machine dependent ioctl()s */

	switch (FREEBSD_IOCGROUP(SCARG(uap, com))) {
	case 'M':
        	freebsd_to_oss(uap, &ap);
		return oss_ioctl_mixer(l, &ap, retval);
	case 'Q':
        	freebsd_to_oss(uap, &ap);
		return oss_ioctl_sequencer(l, &ap, retval);
	case 'P':
        	freebsd_to_oss(uap, &ap);
		return oss_ioctl_audio(l, &ap, retval);
	case 'i':
		freebsd_to_netbsd_ifioctl(uap, &nap);
		return sys_ioctl(l, &nap, retval);
	default:
		return sys_ioctl(l, (const void *)uap, retval);
	}
}
