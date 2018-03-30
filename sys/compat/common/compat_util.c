/* 	$NetBSD: compat_util.c,v 1.46.18.2 2018/03/30 23:57:59 pgoyette Exp $	*/

/*-
 * Copyright (c) 1994 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas and Frank van der Linden.
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
 * Copyright (c) 2008, 2009 Matthew R. Green
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
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
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
__KERNEL_RCSID(0, "$NetBSD: compat_util.c,v 1.46.18.2 2018/03/30 23:57:59 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/filedesc.h>
#include <sys/exec.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/syslog.h>
#include <sys/mount.h>
#include <sys/module.h>

#include <compat/common/compat_util.h>

/*
 * Translate one set of flags to another, based on the entries in
 * the given table.  If 'leftover' is specified, it is filled in
 * with any flags which could not be translated.
 */
unsigned long
emul_flags_translate(const struct emul_flags_xtab *tab,
		     unsigned long in, unsigned long *leftover)
{
	unsigned long out;

	for (out = 0; tab->omask != 0; tab++) {
		if ((in & tab->omask) == tab->oval) {
			in &= ~tab->omask;
			out |= tab->nval;
		}
	}
	if (leftover != NULL)
		*leftover = in;
	return (out);
}

void
compat_offseterr(struct vnode *vp, const char *msg)
{
	struct mount *mp;

	mp = vp->v_mount;

	log(LOG_ERR, "%s: dir offset too large on fs %s (mounted from %s)\n",
	    msg, mp->mnt_stat.f_mntonname, mp->mnt_stat.f_mntfromname);
	uprintf("%s: dir offset too large for emulated program\n", msg);
}

/*
 * Look for native NetBSD compatibility libraries, usually interp-ABI.
 * It returns 0 if it changed the interpreter, otherwise it returns
 * the error from namei().  Callers should not try any more processing
 * if this returns 0, and probably should just ignore the return value.
 */
int
compat_elf_check_interp(struct exec_package *epp,
			char *interp,
			const char *interp_suffix)
{
	int error = 0;

	/*
	 * Don't look for something else, if someone has already found and
	 * setup the ep_interp already.
	 */
	if (interp && epp->ep_interp == NULL) {
		/*
		 * If the path is exactly "/usr/libexec/ld.elf_so", first
		 * try to see if "/usr/libexec/ld.elf_so-<abi>" exists
		 * and if so, use that instead.
		 */
		if (strcmp(interp, "/usr/libexec/ld.elf_so") == 0 ||
		    strcmp(interp, "/libexec/ld.elf_so") == 0) {
			struct vnode *vp;
			char *path;

			path = PNBUF_GET();
			snprintf(path, MAXPATHLEN, "%s-%s", interp, interp_suffix);
			error = namei_simple_kernel(path,
					NSM_FOLLOW_NOEMULROOT, &vp);
			/*
			 * If that worked, replace interpreter in case we
			 * actually need to load it.
			 */
			if (error == 0) {
				epp->ep_interp = vp;
				snprintf(interp, MAXPATHLEN, "%s", path);
			}
			PNBUF_PUT(path);
		}
	}
	return error;
}

#ifdef _MODULE

MODULE(MODULE_CLASS_MISC, compat_util, NULL);

int
compat_util_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
	case MODULE_CMD_FINI:
		return 0;
	default:
		return ENOTTY;
	}
}
#endif	/* _MODULE */
