/*	$NetBSD: irix_swap.c,v 1.2 2002/03/16 20:43:52 christos Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: irix_swap.c,v 1.2 2002/03/16 20:43:52 christos Exp $");

#include <sys/types.h>
#include <sys/signal.h> 
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/swap.h>
#include <sys/syscallargs.h>

#include <compat/common/compat_util.h>

#include <compat/irix/irix_types.h>
#include <compat/irix/irix_signal.h>
#include <compat/irix/irix_swap.h>
#include <compat/irix/irix_syscall.h>
#include <compat/irix/irix_syscallargs.h>

int
irix_sys_swapctl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct irix_sys_swapctl_args /* {
		syscallarg(int) cmd;
		syscallarg(void *) arg;
	} */ *uap = v;
	caddr_t sg = stackgap_init(p, 0);
	struct sys_swapctl_args cup;
	int error = 0;

#ifdef DEBUG_IRIX
	printf("irix_sys_swapctl(): cmd = %d, arg = %p\n", SCARG(uap, cmd),
	    SCARG(uap, arg));
#endif
	SCARG(&cup, arg) = NULL;
	SCARG(&cup, misc) = 0;

	switch (SCARG(uap, cmd)) {
	case IRIX_SC_GETNSWP: /* Get number of swap items */
		SCARG(&cup, cmd) = SWAP_NSWAP;
		return sys_swapctl(p, &cup, retval);
		break;

	case IRIX_SC_LIST: { /* Get swap list */
		struct irix_swaptable ist;
		struct swapent *bse;
		struct irix_swapent *ise, *uise;
		int len, ilen;
		int i;

		if ((error = copyin(SCARG(uap, arg), &ist, sizeof(ist))) != 0)
			return error;

		uise = (struct irix_swapent *)((caddr_t)SCARG(uap, arg) +
		    sizeof(ist.swt_n));

		len = sizeof(struct swapent) * ist.swt_n;
		bse = (struct swapent *)malloc(len, M_TEMP, M_WAITOK);

		ilen = sizeof(struct irix_swapent) * ist.swt_n;
		ise = (struct irix_swapent *)malloc(ilen, M_TEMP, M_WAITOK);

		if ((error = copyin(uise, ise, ilen)) != 0)
			return error;

		SCARG(&cup, cmd) = SWAP_STATS;
		SCARG(&cup, arg) = stackgap_alloc(p, &sg, len);
		SCARG(&cup, misc) = ist.swt_n;
		if ((error = sys_swapctl(p, &cup, retval)) != 0) /*sets retval*/
			goto bad;

		if ((error = copyin(SCARG(&cup, arg), bse, len)) != 0)
			goto bad;
		
		for (i = 0; i < ist.swt_n; i++) {
			if (ise[i].ste_path != NULL && 
			    ((error = copyout(&(bse[i].se_path), 
			    ise[i].ste_path, IRIX_PATH_MAX)) != 0))
				goto bad;
			ise[i].ste_start = 0;
			ise[i].ste_length = bse[i].se_nblks;
			ise[i].ste_pages = bse[i].se_nblks;
			ise[i].ste_free = bse[i].se_nblks - bse[i].se_inuse;

			ise[i].ste_flags = 0; 
			if (bse[i].se_flags & SWF_FAKE)
				ise[i].ste_flags |= IRIX_ST_NOTREADY;

			ise[i].ste_vpages = bse[i].se_inuse;
			ise[i].ste_maxpages = bse[i].se_nblks;
			ise[i].ste_lswap = 1; /* XXX */
			ise[i].ste_pri = bse[i].se_priority;

		}

		error = copyout(ise, uise, ilen);
bad:
		free(bse, M_TEMP);
		free(ise, M_TEMP);
		return error;
		break;
	}

	default:
		printf("irix_sys_swapctl(): unsupported command %d\n", 
		    SCARG(uap, cmd));
		break;
	}
	return 0;
}


