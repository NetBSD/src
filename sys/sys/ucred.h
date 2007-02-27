/*	$NetBSD: ucred.h,v 1.31.10.1 2007/02/27 16:55:19 yamt Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)ucred.h	8.4 (Berkeley) 1/9/95
 */

#ifndef _SYS_UCRED_H_
#define	_SYS_UCRED_H_

#include <sys/param.h>

/*
 * Credentials.
 */

/* Userland's view of credentials. This should not change */
struct uucred {
	u_short		cr_unused;		/* not used, compat */
	uid_t		cr_uid;			/* effective user id */
	gid_t		cr_gid;			/* effective group id */
	short		cr_ngroups;		/* number of groups */
	gid_t		cr_groups[NGROUPS];	/* groups */
};

#ifdef _KERNEL

/*
 * flags that control when do_setres{u,g}id will do anything
 *
 * ID_XXX_EQ_YYY means
 * "allow modifying XXX uid to the given value if the new value of
 * XXX uid (or gid) equals the current value of YYY uid (or gid)."
 */

#define	ID_E_EQ_E	0x001		/* effective equals effective */
#define	ID_E_EQ_R	0x002		/* effective equals real */
#define	ID_E_EQ_S	0x004		/* effective equals saved */
#define	ID_R_EQ_E	0x010		/* real equals effective */
#define	ID_R_EQ_R	0x020		/* real equals real */
#define	ID_R_EQ_S	0x040		/* real equals saved */
#define	ID_S_EQ_E	0x100		/* saved equals effective */
#define	ID_S_EQ_R	0x200		/* saved equals real */
#define	ID_S_EQ_S	0x400		/* saved equals saved */

int		do_setresuid(struct lwp *, uid_t, uid_t, uid_t, u_int);
int		do_setresgid(struct lwp *, gid_t, gid_t, gid_t, u_int);

#endif /* _KERNEL */

#endif /* !_SYS_UCRED_H_ */
