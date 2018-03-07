/*	$NetBSD: subr_emul.c,v 1.1.2.1 2018/03/07 09:33:26 pgoyette Exp $	*/

/*-
 * Copyright (c) 1994, 2000, 2005, 2015 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas and Maxime Villard.
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
 * Copyright (c) 1996 Christopher G. Demetriou
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
 * 3. The name of the author may not be used to endorse or promote products
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
__KERNEL_RCSID(1, "$NetBSD: subr_emul.c,v 1.1.2.1 2018/03/07 09:33:26 pgoyette Exp $");

#ifdef _KERNEL_OPT
#include "opt_pax.h"
#endif /* _KERNEL_OPT */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/exec.h>

#include <compat/common/compat_util.h>

void
emul_find_root(struct lwp *l, struct exec_package *epp)
{
	struct vnode *vp;
	const char *emul_path;

	if (epp->ep_emul_root != NULL)
		/* We've already found it */
		return;

	emul_path = epp->ep_esch->es_emul->e_path;
	if (emul_path == NULL)
		/* Emulation doesn't have a root */
		return;

	if (namei_simple_kernel(emul_path, NSM_FOLLOW_NOEMULROOT, &vp) != 0)
		/* emulation root doesn't exist */
		return;

	epp->ep_emul_root = vp;
}

/*
 * Search the alternate path for dynamic binary interpreter. If not found
 * there, check if the interpreter exists in within 'proper' tree.
 */
int
emul_find_interp(struct lwp *l, struct exec_package *epp, const char *itp)
{
	int error;
	struct pathbuf *pb;
	struct nameidata nd;
	unsigned int flags;

	pb = pathbuf_create(itp);
	if (pb == NULL) {
		return ENOMEM;
	}

	/* If we haven't found the emulation root already, do so now */
	/* Maybe we should remember failures somehow ? */
	if (epp->ep_esch->es_emul->e_path != 0 && epp->ep_emul_root == NULL)
		emul_find_root(l, epp);

	if (epp->ep_interp != NULL)
		vrele(epp->ep_interp);

	/* We need to use the emulation root for the new program,
	 * not the one for the current process. */
	if (epp->ep_emul_root == NULL)
		flags = FOLLOW;
	else {
		nd.ni_erootdir = epp->ep_emul_root;
		/* hack: Pass in the emulation path for ktrace calls */
		nd.ni_next = epp->ep_esch->es_emul->e_path;
		flags = FOLLOW | TRYEMULROOT | EMULROOTSET;
	}

	NDINIT(&nd, LOOKUP, flags, pb);
	error = namei(&nd);
	if (error != 0) {
		epp->ep_interp = NULL;
		pathbuf_destroy(pb);
		return error;
	}

	/* Save interpreter in case we actually need to load it */
	epp->ep_interp = nd.ni_vp;

	pathbuf_destroy(pb);

	return 0;
}
