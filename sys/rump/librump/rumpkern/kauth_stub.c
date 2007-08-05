/*	$NetBSD: kauth_stub.c,v 1.1 2007/08/05 22:28:08 pooka Exp $	*/

/*
 * Copyright (c) 2007 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by Google Summer of Code.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/kauth.h>

int
kauth_authorize_generic(kauth_cred_t cred, kauth_action_t op, void *arg0)
{

	return 0;
}

int
kauth_authorize_system(kauth_cred_t cred, kauth_action_t op,
	enum kauth_system_req req, void *arg1, void *arg2, void *arg3)
{

	return 0;
}

uid_t
kauth_cred_geteuid(kauth_cred_t cred)
{

	return 0;
}

gid_t
kauth_cred_getegid(kauth_cred_t cred)
{

	return 0;
}

int
kauth_cred_ismember_gid(kauth_cred_t cred, gid_t gid, int *resultp)
{

	*resultp = 1;

	return 0;
}

u_int
kauth_cred_ngroups(kauth_cred_t cred)
{

	return 1;
}

gid_t
kauth_cred_group(kauth_cred_t cred, u_int idx)
{

	return 0;
}
