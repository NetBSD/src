/*	$NetBSD: netbsd32_execve.c,v 1.32.34.2 2013/01/23 00:06:02 yamt Exp $	*/

/*
 * Copyright (c) 1998, 2001 Matthew R. Green
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

__KERNEL_RCSID(0, "$NetBSD: netbsd32_execve.c,v 1.32.34.2 2013/01/23 00:06:02 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/atomic.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/stat.h>
#include <sys/spawn.h>
#include <sys/uidinfo.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/syscallargs.h>
#include <sys/proc.h>
#include <sys/exec.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscall.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

static int
netbsd32_execve_fetch_element(char * const *array, size_t index, char **value)
{
	int error;
	netbsd32_charp const *a32 = (void const *)array;
	netbsd32_charp e;

	error = copyin(a32 + index, &e, sizeof(e));
	if (error)
		return error;
	*value = (char *)NETBSD32PTR64(e);
	return 0;
}

int
netbsd32_execve(struct lwp *l, const struct netbsd32_execve_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_charpp) argp;
		syscallarg(netbsd32_charpp) envp;
	} */
	const char *path = SCARG_P32(uap, path);

	return execve1(l, path, SCARG_P32(uap, argp),
	    SCARG_P32(uap, envp), netbsd32_execve_fetch_element);
}

int
netbsd32_fexecve(struct lwp *l, const struct netbsd32_fexecve_args *uap,
		 register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_charpp) argp;
		syscallarg(netbsd32_charpp) envp;
	} */
	struct sys_fexecve_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(argp, char * const);
	NETBSD32TOP_UAP(envp, char * const);

	return sys_fexecve(l, &ua, retval);
}

static int
netbsd32_posix_spawn_fa_alloc(struct posix_spawn_file_actions **fap,
    const struct netbsd32_posix_spawn_file_actions *ufa)
{
	struct posix_spawn_file_actions *fa;
	struct netbsd32_posix_spawn_file_actions fa32;
	struct netbsd32_posix_spawn_file_actions_entry *fae32 = NULL, *f32 = NULL;
	struct posix_spawn_file_actions_entry *fae;
	char *pbuf = NULL;
	int error;
	size_t fal, fal32, slen, i = 0;

	error = copyin(ufa, &fa32, sizeof(fa32));
	if (error)
		return error;

	if (fa32.len == 0)
		return 0;

	fa = kmem_alloc(sizeof(*fa), KM_SLEEP);
	fa->len = fa->size = fa32.len;

	fal = fa->len * sizeof(*fae);
	fal32 = fa->len * sizeof(*fae32);

	fa->fae = kmem_alloc(fal, KM_SLEEP);
	fae32 = kmem_alloc(fal32, KM_SLEEP);
	error = copyin(NETBSD32PTR64(fa32.fae), fae32, fal32);
	if (error)
		goto out;

	pbuf = PNBUF_GET();
	for (; i < fa->len; i++) {
		fae = &fa->fae[i];
		f32 = &fae32[i];
		fae->fae_action = f32->fae_action;
		fae->fae_fildes = f32->fae_fildes;
		if (fae->fae_action == FAE_DUP2)
			fae->fae_data.dup2.newfildes =
			    f32->fae_data.dup2.newfildes;
		if (fae->fae_action != FAE_OPEN)
			continue;
		error = copyinstr(NETBSD32PTR64(f32->fae_path), pbuf,
		    MAXPATHLEN, &slen);
		if (error)
			goto out;
		fae->fae_path = kmem_alloc(slen, KM_SLEEP);
		memcpy(fae->fae_path, pbuf, slen);
		fae->fae_oflag = f32->fae_oflag;
		fae->fae_mode = f32->fae_mode;
	}
	PNBUF_PUT(pbuf);
	if (fae32)
		kmem_free(fae32, fal32);
	*fap = fa;
	return 0;

out:
	if (fae32)
		kmem_free(fae32, fal32);
	if (pbuf)
		PNBUF_PUT(pbuf);
	posix_spawn_fa_free(fa, i);
	return error;
}

int
netbsd32_posix_spawn(struct lwp *l,
	const struct netbsd32_posix_spawn_args *uap, register_t *retval)
{
	/* {
	syscallarg(netbsd32_pid_tp) pid;
	syscallarg(const netbsd32_charp) path;
	syscallarg(const netbsd32_posix_spawn_file_actionsp) file_actions;
	syscallarg(const netbsd32_posix_spawnattrp) attrp;
	syscallarg(netbsd32_charpp) argv;
	syscallarg(netbsd32_charpp) envp;
	} */

	int error;
	struct posix_spawn_file_actions *fa = NULL;
	struct posix_spawnattr *sa = NULL;
	pid_t pid;
	bool child_ok = false;

	error = check_posix_spawn(l);
	if (error) {
		*retval = error;
		return 0;
	}

	/* copy in file_actions struct */
	if (SCARG_P32(uap, file_actions) != NULL) {
		error = netbsd32_posix_spawn_fa_alloc(&fa,
		    SCARG_P32(uap, file_actions));
		if (error)
			goto error_exit;
	}

	/* copyin posix_spawnattr struct */
	if (SCARG_P32(uap, attrp) != NULL) {
		sa = kmem_alloc(sizeof(*sa), KM_SLEEP);
		error = copyin(SCARG_P32(uap, attrp), sa, sizeof(*sa));
		if (error)
			goto error_exit;
	}

	/*
	 * Do the spawn
	 */
	error = do_posix_spawn(l, &pid, &child_ok, SCARG_P32(uap, path), fa,
	    sa, SCARG_P32(uap, argv), SCARG_P32(uap, envp),
	    netbsd32_execve_fetch_element);
	if (error)
		goto error_exit;

	if (error == 0 && SCARG_P32(uap, pid) != NULL)
		error = copyout(&pid, SCARG_P32(uap, pid), sizeof(pid));

	*retval = error;
	return 0;

 error_exit:
 	if (!child_ok) {
		(void)chgproccnt(kauth_cred_getuid(l->l_cred), -1);
		atomic_dec_uint(&nprocs);

		if (sa)
			kmem_free(sa, sizeof(*sa));
		if (fa)
			posix_spawn_fa_free(fa, fa->len);
	}

	*retval = error;
	return 0;
}
