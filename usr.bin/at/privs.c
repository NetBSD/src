/*	$NetBSD: privs.c,v 1.3 2018/02/25 23:48:16 htodd Exp $	*/

/*
 *  privs.c - privileged operations
 *  Copyright (C) 1993  Thomas Koenig
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author(s) may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * From: OpenBSD: privs.h,v 1.4 1997/03/01 23:40:12 millert Exp
 */

#include <unistd.h>

#include "privs.h"

/*
 * Used by: usr.bin/at
 * Used by: libexec/atrun
 */

/*
 * Relinquish privileges temporarily for a setuid or setgid program
 * with the option of getting them back later.  This is done by
 * using POSIX saved user and groups ids.  Call RELINQUISH_PRIVS once
 * at the beginning of the main program.  This will cause all operations
 * to be executed with the real userid.  When you need the privileges
 * of the setuid/setgid invocation, call PRIV_START; when you no longer
 * need it, call PRIV_END.  Note that it is an error to call PRIV_START
 * and not PRIV_END within the same function.
 *
 * Use RELINQUISH_PRIVS_ROOT(a,b) if your program started out running
 * as root, and you want to drop back the effective userid to a
 * and the effective group id to b, with the option to get them back
 * later.
 *
 * Problems: Do not use return between PRIV_START and PRIV_END; this
 * will cause the program to continue running in an unprivileged
 * state.
 *
 * It is NOT safe to call exec(), system() or popen() with a user-
 * supplied program (i.e. without carefully checking PATH and any
 * library load paths) with relinquished privileges; the called program
 * can acquire them just as easily.  Set both effective and real userid
 * to the real userid before calling any of them.
 */

uid_t real_uid, effective_uid;
gid_t real_gid, effective_gid;

void
privs_enter(void)
{
	if (seteuid(effective_uid) == -1)
		privs_fail("Cannot get user privs");
	if (setegid(effective_gid) == -1)
		privs_fail("Cannot get group privs");
}

void
privs_exit(void)
{
	if (setegid(real_gid) == -1)
		privs_fail("Cannot relinquish group privs");
	if (seteuid(real_uid) == -1)
		privs_fail("Cannot relinquish user privs");
}

void
privs_relinquish(void)
{
	real_uid = getuid();
	effective_uid = geteuid();
	real_gid = getgid();
	effective_gid = getegid();
	privs_exit();
}

void
privs_relinquish_root(uid_t ruid, gid_t rgid)
{
	real_uid = ruid;
	real_gid = rgid;
	effective_uid = geteuid();
	effective_gid = getegid();
	privs_exit();
}
