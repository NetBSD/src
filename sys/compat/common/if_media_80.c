/*	$NetBSD: if_media_80.c,v 1.1.4.1 2022/08/03 11:11:31 martin Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

/*
 * Copyright (c) 1997
 *	Jonathan Stone and Jason R. Thorpe.  All rights reserved.
 *
 * This software is derived from information provided by Matt Thomas.
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
 *      This product includes software developed by Jonathan Stone
 *	and Jason R. Thorpe for the NetBSD Project.
 * 4. The names of the authors may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/syscallargs.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/compat_stub.h>

#include <net/if.h>
#include <net/if_media.h>

#include <compat/sys/sockio.h>
#include <compat/common/compat_mod.h>

static void
ifmword_n2o(int *oldwd, int *newwd)
{

	if (IFM_SUBTYPE(*newwd) > IFM_OTHER)
		*oldwd = (*newwd & ~(_IFM_ETH_XTMASK | IFM_TMASK)) | IFM_OTHER;
	else
		*oldwd = *newwd;
}

/*ARGSUSED*/
static int
compat_ifmediareq_pre(struct ifreq *ifr, u_long *cmd, bool *do_post)
{
	struct ifmediareq *ifmr = (struct ifmediareq *)ifr;

	switch (*cmd) {
	case SIOCSIFMEDIA_80:
		*cmd = SIOCSIFMEDIA; /* Convert to new one */
		if ((IFM_TYPE(ifr->ifr_media) == IFM_ETHER) &&
		    IFM_SUBTYPE(ifr->ifr_media) > IFM_OTHER) {
			/* Clear unused bits to not to change to wrong media */
			ifr->ifr_media &= ~_IFM_ETH_XTMASK;
		}
		return 0;
	case SIOCGIFMEDIA_80:
		*cmd = SIOCGIFMEDIA; /* Convert to new one */
		if (ifmr->ifm_count != 0) {
			/*
			 * Tell the upper layer to try to convert each ifmedia
			 * entry in the post process.
			 */
			*do_post = true;
		}
		return 0;
	default:
		return 0;
	}
}

/*ARGSUSED*/
static int
compat_ifmediareq_post(struct ifreq *ifr, u_long cmd)
{
	struct ifmediareq *ifmr = (struct ifmediareq *)ifr;
	size_t minwords;
	int count, *kptr;
	int error;

	switch (cmd) {
	case SIOCSIFMEDIA:
		return 0;
	case SIOCGIFMEDIA:
		if (ifmr->ifm_count < 0)
			return EINVAL;
		
		/*
		 * ifmr->ifm_count was already ajusted in ifmedia_ioctl(), so
		 * there is no problem to trust ifm_count.
		 */
		minwords = ifmr->ifm_count;
		kptr = malloc(minwords * sizeof(*kptr), M_TEMP,
		    M_WAITOK|M_ZERO);
		if (kptr == NULL)
			return ENOMEM;

		/*
		 * Convert ifm_current and ifm_active.
		 * It's not required to convert ifm_mask.
		 */
		ifmword_n2o(&ifmr->ifm_current, &ifmr->ifm_current);
		ifmword_n2o(&ifmr->ifm_active, &ifmr->ifm_active);

		/* Convert ifm_ulist array */
		for (count = 0; count < minwords; count++) {
			int oldmwd;

			error = ufetch_int(&ifmr->ifm_ulist[count], &oldmwd);
			if (error != 0)
				goto out;
			ifmword_n2o(&kptr[count], &oldmwd);
		}

		/* Copy to userland in old format */
		error = copyout(kptr, ifmr->ifm_ulist,
		    minwords * sizeof(*kptr));
out:
		free(kptr, M_TEMP);
		return error;
	default:
		return 0;
	}
}

void
ifmedia_80_init(void)
{

	MODULE_HOOK_SET(ifmedia_80_pre_hook, "ifmedia80",
	    compat_ifmediareq_pre);
	MODULE_HOOK_SET(ifmedia_80_post_hook, "ifmedia80",
	    compat_ifmediareq_post);
}

void
ifmedia_80_fini(void)
{
 
	MODULE_HOOK_UNSET(ifmedia_80_post_hook);
	MODULE_HOOK_UNSET(ifmedia_80_pre_hook);
}
