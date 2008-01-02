/*	$NetBSD: exec_script.c,v 1.61 2008/01/02 19:44:37 yamt Exp $	*/

/*
 * Copyright (c) 1993, 1994, 1996 Christopher G. Demetriou
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
 *      This product includes software developed by Christopher G. Demetriou.
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
__KERNEL_RCSID(0, "$NetBSD: exec_script.c,v 1.61 2008/01/02 19:44:37 yamt Exp $");

#if defined(SETUIDSCRIPTS) && !defined(FDSCRIPTS)
#define FDSCRIPTS		/* Need this for safe set-id scripts. */
#endif

#include "veriexec.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/kmem.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/file.h>
#ifdef SETUIDSCRIPTS
#include <sys/stat.h>
#endif
#include <sys/filedesc.h>
#include <sys/exec.h>
#include <sys/resourcevar.h>

#include <sys/exec_script.h>
#include <sys/exec_elf.h>

/*
 * exec_script_makecmds(): Check if it's an executable shell script.
 *
 * Given a proc pointer and an exec package pointer, see if the referent
 * of the epp is in shell script.  If it is, then set thing up so that
 * the script can be run.  This involves preparing the address space
 * and arguments for the shell which will run the script.
 *
 * This function is ultimately responsible for creating a set of vmcmds
 * which can be used to build the process's vm space and inserting them
 * into the exec package.
 */
int
exec_script_makecmds(struct lwp *l, struct exec_package *epp)
{
	int error, hdrlinelen, shellnamelen, shellarglen;
	char *hdrstr = epp->ep_hdr;
	char *cp, *shellname, *shellarg, *oldpnbuf;
	size_t shellargp_len;
	struct exec_fakearg *shellargp;
	struct exec_fakearg *tmpsap;
	struct vnode *scriptvp;
#ifdef SETUIDSCRIPTS
	/* Gcc needs those initialized for spurious uninitialized warning */
	uid_t script_uid = (uid_t) -1;
	gid_t script_gid = NOGROUP;
	u_short script_sbits;
#endif

	/*
	 * if the magic isn't that of a shell script, or we've already
	 * done shell script processing for this exec, punt on it.
	 */
	if ((epp->ep_flags & EXEC_INDIR) != 0 ||
	    epp->ep_hdrvalid < EXEC_SCRIPT_MAGICLEN ||
	    strncmp(hdrstr, EXEC_SCRIPT_MAGIC, EXEC_SCRIPT_MAGICLEN))
		return ENOEXEC;

	/*
	 * check that the shell spec is terminated by a newline,
	 * and that it isn't too large.  Don't modify the
	 * buffer unless we're ready to commit to handling it.
	 * (The latter requirement means that we have to check
	 * for both spaces and tabs later on.)
	 */
	hdrlinelen = min(epp->ep_hdrvalid, SCRIPT_HDR_SIZE);
	for (cp = hdrstr + EXEC_SCRIPT_MAGICLEN; cp < hdrstr + hdrlinelen;
	    cp++) {
		if (*cp == '\n') {
			*cp = '\0';
			break;
		}
	}
	if (cp >= hdrstr + hdrlinelen)
		return ENOEXEC;

	/*
	 * If the script has an ELF header, don't exec it.
	 */
	if (epp->ep_hdrvalid >= sizeof(ELFMAG)-1 &&
	    memcmp(hdrstr, ELFMAG, sizeof(ELFMAG)-1) == 0)
		return ENOEXEC;

	shellname = NULL;
	shellarg = NULL;
	shellarglen = 0;

	/* strip spaces before the shell name */
	for (cp = hdrstr + EXEC_SCRIPT_MAGICLEN; *cp == ' ' || *cp == '\t';
	    cp++)
		;

	/* collect the shell name; remember it's length for later */
	shellname = cp;
	shellnamelen = 0;
	if (*cp == '\0')
		goto check_shell;
	for ( /* cp = cp */ ; *cp != '\0' && *cp != ' ' && *cp != '\t'; cp++)
		shellnamelen++;
	if (*cp == '\0')
		goto check_shell;
	*cp++ = '\0';

	/* skip spaces before any argument */
	for ( /* cp = cp */ ; *cp == ' ' || *cp == '\t'; cp++)
		;
	if (*cp == '\0')
		goto check_shell;

	/*
	 * collect the shell argument.  everything after the shell name
	 * is passed as ONE argument; that's the correct (historical)
	 * behaviour.
	 */
	shellarg = cp;
	for ( /* cp = cp */ ; *cp != '\0'; cp++)
		shellarglen++;
	*cp++ = '\0';

check_shell:
#ifdef SETUIDSCRIPTS
	/*
	 * MNT_NOSUID has already taken care of by check_exec,
	 * so we don't need to worry about it now or later.  We
	 * will need to check PSL_TRACED later, however.
	 */
	script_sbits = epp->ep_vap->va_mode & (S_ISUID | S_ISGID);
	if (script_sbits != 0) {
		script_uid = epp->ep_vap->va_uid;
		script_gid = epp->ep_vap->va_gid;
	}
#endif
#ifdef FDSCRIPTS
	/*
	 * if the script isn't readable, or it's set-id, then we've
	 * gotta supply a "/dev/fd/..." for the shell to read.
	 * Note that stupid shells (csh) do the wrong thing, and
	 * close all open fd's when the start.  That kills this
	 * method of implementing "safe" set-id and x-only scripts.
	 */
	vn_lock(epp->ep_vp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_ACCESS(epp->ep_vp, VREAD, l->l_cred);
	VOP_UNLOCK(epp->ep_vp, 0);
	if (error == EACCES
#ifdef SETUIDSCRIPTS
	    || script_sbits
#endif
	    ) {
		struct file *fp;

#if defined(DIAGNOSTIC) && defined(FDSCRIPTS)
		if (epp->ep_flags & EXEC_HASFD)
			panic("exec_script_makecmds: epp already has a fd");
#endif

		/* falloc() will use the descriptor for us */
		if ((error = falloc(l, &fp, &epp->ep_fd)) != 0) {
			scriptvp = NULL;
			shellargp = NULL;
			goto fail;
		}

		epp->ep_flags |= EXEC_HASFD;
		fp->f_type = DTYPE_VNODE;
		fp->f_ops = &vnops;
		fp->f_data = (void *) epp->ep_vp;
		fp->f_flag = FREAD;
		FILE_SET_MATURE(fp);
		FILE_UNUSE(fp, l);
	}
#endif

	/* set up the parameters for the recursive check_exec() call */
	epp->ep_ndp->ni_dirp = shellname;
	epp->ep_ndp->ni_segflg = UIO_SYSSPACE;
	epp->ep_flags |= EXEC_INDIR;

	/* and set up the fake args list, for later */
	shellargp_len = 4 * sizeof(*shellargp);
	shellargp = kmem_alloc(shellargp_len, KM_SLEEP);
	tmpsap = shellargp;
	tmpsap->fa_len = shellnamelen + 1;
	tmpsap->fa_arg = kmem_alloc(tmpsap->fa_len, KM_SLEEP);
	strlcpy(tmpsap->fa_arg, shellname, tmpsap->fa_len);
	tmpsap++;
	if (shellarg != NULL) {
		tmpsap->fa_len = shellarglen + 1;
		tmpsap->fa_arg = kmem_alloc(tmpsap->fa_len, KM_SLEEP);
		strlcpy(tmpsap->fa_arg, shellarg, tmpsap->fa_len);
		tmpsap++;
	}
	tmpsap->fa_len = MAXPATHLEN;
	tmpsap->fa_arg = kmem_alloc(tmpsap->fa_len, KM_SLEEP);
#ifdef FDSCRIPTS
	if ((epp->ep_flags & EXEC_HASFD) == 0) {
#endif
		/* normally can't fail, but check for it if diagnostic */
		error = copyinstr(epp->ep_name, tmpsap->fa_arg, MAXPATHLEN,
		    (size_t *)0);
		tmpsap++;
#ifdef DIAGNOSTIC
		if (error != 0)
			panic("exec_script: copyinstr couldn't fail");
#endif
#ifdef FDSCRIPTS
	} else {
		snprintf(tmpsap->fa_arg, MAXPATHLEN, "/dev/fd/%d", epp->ep_fd);
		tmpsap++;
	}
#endif
	tmpsap->fa_arg = NULL;

	/*
	 * mark the header we have as invalid; check_exec will read
	 * the header from the new executable
	 */
	epp->ep_hdrvalid = 0;

	/*
	 * remember the old vp and pnbuf for later, so we can restore
	 * them if check_exec() fails.
	 */
	scriptvp = epp->ep_vp;
	oldpnbuf = epp->ep_ndp->ni_cnd.cn_pnbuf;

	error = check_exec(l, epp);
	/* note that we've clobbered the header */
	epp->ep_flags |= EXEC_DESTR;
	if (error == 0) {
		/*
		 * It succeeded.  Unlock the script and
		 * close it if we aren't using it any more.
		 * Also, set things up so that the fake args
		 * list will be used.
		 */
		if ((epp->ep_flags & EXEC_HASFD) == 0) {
			vn_lock(scriptvp, LK_EXCLUSIVE | LK_RETRY);
			VOP_CLOSE(scriptvp, FREAD, l->l_cred);
			vput(scriptvp);
		}

		/* free the old pathname buffer */
		PNBUF_PUT(oldpnbuf);

		epp->ep_flags |= (EXEC_HASARGL | EXEC_SKIPARG);
		epp->ep_fa = shellargp;
		epp->ep_fa_len = shellargp_len;
#ifdef SETUIDSCRIPTS
		/*
		 * set thing up so that set-id scripts will be
		 * handled appropriately.  PSL_TRACED will be
		 * checked later when the shell is actually
		 * exec'd.
		 */
		epp->ep_vap->va_mode |= script_sbits;
		if (script_sbits & S_ISUID)
			epp->ep_vap->va_uid = script_uid;
		if (script_sbits & S_ISGID)
			epp->ep_vap->va_gid = script_gid;
#endif
		return (0);
	}

	/* XXX oldpnbuf not set for "goto fail" path */
	epp->ep_ndp->ni_cnd.cn_pnbuf = oldpnbuf;
#ifdef FDSCRIPTS
fail:
#endif

	/* kill the opened file descriptor, else close the file */
        if (epp->ep_flags & EXEC_HASFD) {
                epp->ep_flags &= ~EXEC_HASFD;
                (void) fdrelease(l, epp->ep_fd);
        } else if (scriptvp) {
		vn_lock(scriptvp, LK_EXCLUSIVE | LK_RETRY);
		VOP_CLOSE(scriptvp, FREAD, l->l_cred);
		vput(scriptvp);
	}

        PNBUF_PUT(epp->ep_ndp->ni_cnd.cn_pnbuf);

	/* free the fake arg list, because we're not returning it */
	if ((tmpsap = shellargp) != NULL) {
		while (tmpsap->fa_arg != NULL) {
			kmem_free(tmpsap->fa_arg, tmpsap->fa_len);
			tmpsap++;
		}
		kmem_free(shellargp, shellargp_len);
	}

        /*
         * free any vmspace-creation commands,
         * and release their references
         */
        kill_vmcmds(&epp->ep_vmcmds);

        return error;
}
