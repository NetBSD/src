/*	$NetBSD: aoutm68k_stat.c,v 1.2.4.1 2001/04/09 01:55:28 nathanw Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#if defined(_KERNEL) && !defined(_LKM)
#include "opt_compat_netbsd.h"
#include "opt_compat_43.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/stat.h>

#include <sys/syscall.h>
#include <sys/syscallargs.h>

#include <compat/aoutm68k/aoutm68k_util.h>
#include <compat/aoutm68k/aoutm68k_stat.h>
#include <compat/aoutm68k/aoutm68k_syscall.h>
#include <compat/aoutm68k/aoutm68k_syscallargs.h>

#ifdef COMPAT_43
static void aoutm68k_stat43_convert(struct stat43 *, struct aoutm68k_stat43 *);
#endif
#ifdef COMPAT_12
static void aoutm68k_stat12_convert(struct stat12 *, struct aoutm68k_stat12 *);
#endif
static void aoutm68k_stat13_convert(struct stat *, struct aoutm68k_stat *);


#ifdef COMPAT_43
int
aoutm68k_compat_43_sys_stat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct aoutm68k_compat_43_sys_stat_args *uap = v;
	caddr_t sg = stackgap_init(p->p_emul);
	struct compat_43_sys_stat_args cup;
	struct aoutm68k_stat43 ast;
	struct stat43 st;
	int error;

	SCARG(&cup, ub) = stackgap_alloc(&sg, sizeof(st));
#ifdef COMPAT_AOUT_ALTPATH
	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
#endif
	SCARG(&cup, path) = SCARG(uap, path);

	if ((error = compat_43_sys_stat(p, &cup, retval)) != 0 ||
	    (error = copyin(SCARG(&cup, ub), &st, sizeof(st))) != 0)
		return (error);

	aoutm68k_stat43_convert(&st, &ast);

	return (copyout((caddr_t)&ast, (caddr_t)SCARG(uap, ub), sizeof(ast)));
}

int
aoutm68k_compat_43_sys_fstat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct aoutm68k_compat_43_sys_fstat_args *uap = v;
	caddr_t sg = stackgap_init(p->p_emul);
	struct compat_43_sys_fstat_args cup;
	struct aoutm68k_stat43 ast;
	struct stat43 st;
	int error;

	SCARG(&cup, fd) = SCARG(uap, fd);
	SCARG(&cup, sb) = stackgap_alloc(&sg, sizeof(st));

	if ((error = compat_43_sys_fstat(p, &cup, retval)) != 0 ||
	    (error = copyin(SCARG(&cup, sb), &st, sizeof(st))) != 0)
		return (error);

	aoutm68k_stat43_convert(&st, &ast);

	return (copyout((caddr_t)&ast, (caddr_t)SCARG(uap, sb), sizeof(ast)));
}

int
aoutm68k_compat_43_sys_lstat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct aoutm68k_compat_43_sys_lstat_args *uap = v;
	caddr_t sg = stackgap_init(p->p_emul);
	struct compat_43_sys_lstat_args cup;
	struct aoutm68k_stat43 ast;
	struct stat43 st;
	int error;

	SCARG(&cup, ub) = stackgap_alloc(&sg, sizeof(st));
#ifdef COMPAT_AOUT_ALTPATH
	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
#endif
	SCARG(&cup, path) = SCARG(uap, path);

	if ((error = compat_43_sys_lstat(p, &cup, retval)) != 0 ||
	    (error = copyin(SCARG(&cup, ub), &st, sizeof(st))) != 0)
		return (error);

	aoutm68k_stat43_convert(&st, &ast);

	return (copyout((caddr_t)&ast, (caddr_t)SCARG(uap, ub), sizeof(ast)));
}
#endif /* COMPAT_43 */

#ifdef COMPAT_12
int
aoutm68k_compat_12_sys_stat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct aoutm68k_compat_12_sys_stat_args *uap = v;
	caddr_t sg = stackgap_init(p->p_emul);
	struct compat_12_sys_stat_args cup;
	struct aoutm68k_stat12 ast;
	struct stat12 st;
	int error;

	SCARG(&cup, ub) = stackgap_alloc(&sg, sizeof(st));
#ifdef COMPAT_AOUT_ALTPATH
	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
#endif
	SCARG(&cup, path) = SCARG(uap, path);

	if ((error = compat_12_sys_stat(p, &cup, retval)) != 0 ||
	    (error = copyin(SCARG(&cup, ub), &st, sizeof(st))) != 0)
		return (error);

	aoutm68k_stat12_convert(&st, &ast);

	return (copyout((caddr_t)&ast, (caddr_t)SCARG(uap, ub), sizeof(ast)));
}

int
aoutm68k_compat_12_sys_fstat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct aoutm68k_compat_12_sys_fstat_args *uap = v;
	caddr_t sg = stackgap_init(p->p_emul);
	struct compat_12_sys_fstat_args cup;
	struct aoutm68k_stat12 ast;
	struct stat12 st;
	int error;

	SCARG(&cup, fd) = SCARG(uap, fd);
	SCARG(&cup, sb) = stackgap_alloc(&sg, sizeof(st));

	if ((error = compat_12_sys_fstat(p, &cup, retval)) != 0 ||
	    (error = copyin(SCARG(&cup, sb), &st, sizeof(st))) != 0)
		return (error);

	aoutm68k_stat12_convert(&st, &ast);

	return (copyout((caddr_t)&ast, (caddr_t)SCARG(uap, sb), sizeof(ast)));
}

int
aoutm68k_compat_12_sys_lstat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct aoutm68k_compat_12_sys_lstat_args *uap = v;
	caddr_t sg = stackgap_init(p->p_emul);
	struct compat_12_sys_lstat_args cup;
	struct aoutm68k_stat12 ast;
	struct stat12 st;
	int error;

	SCARG(&cup, ub) = stackgap_alloc(&sg, sizeof(st));
#ifdef COMPAT_AOUT_ALTPATH
	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
#endif
	SCARG(&cup, path) = SCARG(uap, path);

	if ((error = compat_12_sys_lstat(p, &cup, retval)) != 0 ||
	    (error = copyin(SCARG(&cup, ub), &st, sizeof(st))) != 0)
		return (error);

	aoutm68k_stat12_convert(&st, &ast);

	return (copyout((caddr_t)&ast, (caddr_t)SCARG(uap, ub), sizeof(ast)));
}
#endif /* COMPAT_12 */

int
aoutm68k_sys___stat13(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct aoutm68k_sys___stat13_args *uap = v;
	caddr_t sg = stackgap_init(p->p_emul);
	struct sys___stat13_args cup;
	struct aoutm68k_stat ast;
	struct stat st;
	int error;

	SCARG(&cup, ub) = stackgap_alloc(&sg, sizeof(st));
#ifdef COMPAT_AOUT_ALTPATH
	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
#endif
	SCARG(&cup, path) = SCARG(uap, path);

	if ((error = sys___stat13(p, &cup, retval)) != 0 ||
	    (error = copyin(SCARG(&cup, ub), &st, sizeof(st))) != 0)
		return (error);

	aoutm68k_stat13_convert(&st, &ast);

	return (copyout((caddr_t)&ast, (caddr_t)SCARG(uap, ub), sizeof(ast)));
}

int
aoutm68k_sys___fstat13(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct aoutm68k_sys___fstat13_args *uap = v;
	caddr_t sg = stackgap_init(p->p_emul);
	struct sys___fstat13_args cup;
	struct aoutm68k_stat ast;
	struct stat st;
	int error;

	SCARG(&cup, fd) = SCARG(uap, fd);
	SCARG(&cup, sb) = stackgap_alloc(&sg, sizeof(st));

	if ((error = sys___fstat13(p, &cup, retval)) != 0 ||
	    (error = copyin(SCARG(&cup, sb), &st, sizeof(st))) != 0)
		return (error);

	aoutm68k_stat13_convert(&st, &ast);

	return (copyout((caddr_t)&ast, (caddr_t)SCARG(uap, sb), sizeof(ast)));

}

int
aoutm68k_sys___lstat13(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct aoutm68k_sys___lstat13_args *uap = v;
	caddr_t sg = stackgap_init(p->p_emul);
	struct sys___lstat13_args cup;
	struct aoutm68k_stat ast;
	struct stat st;
	int error;

	SCARG(&cup, ub) = stackgap_alloc(&sg, sizeof(st));
#ifdef COMPAT_AOUT_ALTPATH
	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
#endif
	SCARG(&cup, path) = SCARG(uap, path);

	if ((error = sys___lstat13(p, &cup, retval)) != 0 ||
	    (error = copyin(SCARG(&cup, ub), &st, sizeof(st))) != 0)
		return (error);

	aoutm68k_stat13_convert(&st, &ast);

	return (copyout((caddr_t)&ast, (caddr_t)SCARG(uap, ub), sizeof(ast)));
}

int
aoutm68k_sys_fhstat(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct aoutm68k_sys_fhstat_args *uap = v;
	caddr_t sg = stackgap_init(p->p_emul);
	struct sys_fhstat_args cup;
	struct aoutm68k_stat ast;
	struct stat st;
	int error;

	SCARG(&cup, fhp) = SCARG(uap, fhp);
	SCARG(&cup, sb) = stackgap_alloc(&sg, sizeof(st));

	if ((error = sys_fhstat(p, &cup, retval)) != 0 ||
	    (error = copyin(SCARG(&cup, sb), &st, sizeof(st))) != 0)
		return (error);

	aoutm68k_stat13_convert(&st, &ast);

	return (copyout((caddr_t)&ast, (caddr_t)SCARG(uap, sb), sizeof(ast)));
}

#ifdef COMPAT_43
static void
aoutm68k_stat43_convert(st, ast)
	struct stat43 *st;
	struct aoutm68k_stat43 *ast;
{

	memset(ast, 0, sizeof(*ast));
	ast->st_dev = st->st_dev;
	ast->st_ino = st->st_ino;
	ast->st_mode = st->st_mode;
	ast->st_nlink = st->st_nlink;
	ast->st_uid = st->st_uid;
	ast->st_gid = st->st_gid;
	ast->st_rdev = st->st_rdev;
	ast->st_size = st->st_size;
	ast->st_atimespec.tv_sec = st->st_atimespec.tv_sec;
	ast->st_atimespec.tv_nsec = st->st_atimespec.tv_nsec;
	ast->st_mtimespec.tv_sec = st->st_mtimespec.tv_sec;
	ast->st_mtimespec.tv_nsec = st->st_mtimespec.tv_nsec;
	ast->st_ctimespec.tv_sec = st->st_ctimespec.tv_sec;
	ast->st_ctimespec.tv_nsec = st->st_ctimespec.tv_nsec;
	ast->st_blksize = st->st_blksize;
	ast->st_blocks = st->st_blocks;
	ast->st_flags = st->st_flags;
	ast->st_gen = st->st_gen;
}
#endif /* COMPAT_43 */

#ifdef COMPAT_12
static void
aoutm68k_stat12_convert(st, ast)
	struct stat12 *st;
	struct aoutm68k_stat12 *ast;
{

	memset(ast, 0, sizeof(*ast));
	ast->st_dev = st->st_dev;
	ast->st_ino = st->st_ino;
	ast->st_mode = st->st_mode;
	ast->st_nlink = st->st_nlink;
	ast->st_uid = st->st_uid;
	ast->st_gid = st->st_gid;
	ast->st_rdev = st->st_rdev;
	ast->st_atimespec.tv_sec = st->st_atimespec.tv_sec;
	ast->st_atimespec.tv_nsec = st->st_atimespec.tv_nsec;
	ast->st_mtimespec.tv_sec = st->st_mtimespec.tv_sec;
	ast->st_mtimespec.tv_nsec = st->st_mtimespec.tv_nsec;
	ast->st_ctimespec.tv_sec = st->st_ctimespec.tv_sec;
	ast->st_ctimespec.tv_nsec = st->st_ctimespec.tv_nsec;
	ast->st_size = st->st_size;
	ast->st_blocks = st->st_blocks;
	ast->st_blksize = st->st_blksize;
	ast->st_flags = st->st_flags;
	ast->st_gen = st->st_gen;
	ast->st_lspare = st->st_lspare;
	ast->st_qspare[0] = st->st_qspare[0];
	ast->st_qspare[1] = st->st_qspare[1];
}
#endif /* COMPAT_12 */

static void
aoutm68k_stat13_convert(st, ast)
	struct stat *st;
	struct aoutm68k_stat *ast;
{

	memset(ast, 0, sizeof(*ast));
	ast->st_dev = st->st_dev;
	ast->st_ino = st->st_ino;
	ast->st_mode = st->st_mode;
	ast->st_nlink = st->st_nlink;
	ast->st_uid = st->st_uid;
	ast->st_gid = st->st_gid;
	ast->st_rdev = st->st_rdev;
	ast->st_atimespec.tv_sec = st->st_atimespec.tv_sec;
	ast->st_atimespec.tv_nsec = st->st_atimespec.tv_nsec;
	ast->st_mtimespec.tv_sec = st->st_mtimespec.tv_sec;
	ast->st_mtimespec.tv_nsec = st->st_mtimespec.tv_nsec;
	ast->st_ctimespec.tv_sec = st->st_ctimespec.tv_sec;
	ast->st_ctimespec.tv_nsec = st->st_ctimespec.tv_nsec;
	ast->st_size = st->st_size;
	ast->st_blocks = st->st_blocks;
	ast->st_blksize = st->st_blksize;
	ast->st_flags = st->st_flags;
	ast->st_gen = st->st_gen;
	ast->st_qspare[0] = st->st_qspare[0];
	ast->st_qspare[1] = st->st_qspare[1];
}
