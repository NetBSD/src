/*	$NetBSD: irix_sysctl.c,v 1.1 2002/11/09 09:03:58 manu Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: irix_sysctl.c,v 1.1 2002/11/09 09:03:58 manu Exp $");

#include <sys/param.h>
#include <sys/signal.h>
#include <sys/sysctl.h>

#include <compat/irix/irix_sysctl.h>

int
irix_sysctl(int *name, u_int nlen, void *oldp, size_t *oldlenp,
    void *newp, size_t newlen, struct proc *p)
{
	if (nlen != 2 || name[0] != EMUL_IRIX_KERN)
		return EOPNOTSUPP;

	switch (name[1]) {
	case EMUL_IRIX_KERN_VENDOR:
		return sysctl_string(oldp, oldlenp, newp, newlen,
		    irix_si_vendor, sizeof(irix_si_vendor));
	case EMUL_IRIX_KERN_OSPROVIDER:
		return sysctl_string(oldp, oldlenp, newp, newlen,
		    irix_si_os_provider, sizeof(irix_si_os_provider));
	case EMUL_IRIX_KERN_OSNAME:
		return sysctl_string(oldp, oldlenp, newp, newlen,
		    irix_si_os_name, sizeof(irix_si_os_name));
	case EMUL_IRIX_KERN_HWNAME:
		return sysctl_string(oldp, oldlenp, newp, newlen,
		    irix_si_hw_name, sizeof(irix_si_hw_name));
	case EMUL_IRIX_KERN_OSRELMAJ:
		return sysctl_string(oldp, oldlenp, newp, newlen,
		    irix_si_osrel_maj, sizeof(irix_si_osrel_maj));
	case EMUL_IRIX_KERN_OSRELMIN:
		return sysctl_string(oldp, oldlenp, newp, newlen,
		    irix_si_osrel_min, sizeof(irix_si_osrel_min));
	case EMUL_IRIX_KERN_OSRELPATCH:
		return sysctl_string(oldp, oldlenp, newp, newlen,
		    irix_si_osrel_patch, sizeof(irix_si_osrel_patch));
	case EMUL_IRIX_KERN_PROCESSOR:
		return sysctl_string(oldp, oldlenp, newp, newlen,
		    irix_si_processors, sizeof(irix_si_processors));
	case EMUL_IRIX_KERN_VERSION:
		return sysctl_string(oldp, oldlenp, newp, newlen,
		    irix_si_version, sizeof(irix_si_version));
	default:
		return EOPNOTSUPP;
	}
}
