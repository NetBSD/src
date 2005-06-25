/*	$NetBSD: register.c,v 1.11 2005/06/25 12:17:57 elad Exp $	*/
/*	$OpenBSD: register.c,v 1.11 2002/08/05 14:49:27 provos Exp $	*/
/*
 * Copyright 2002 Niels Provos <provos@citi.umich.edu>
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
 *      This product includes software developed by Niels Provos.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/tree.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <err.h>

#include "intercept.h"
#include "systrace.h"

#define X(x)	if ((x) == -1) \
	err(1, "%s:%d: intercept failed", __func__, __LINE__)

void
systrace_initcb(void)
{
	struct systrace_alias *alias;
	struct intercept_translate *tl;

	X(intercept_init());

#ifdef __NetBSD__
	X(intercept_register_gencb(gen_cb, NULL));

	/* 5: open [fswrite] */
	X(intercept_register_sccb("netbsd", "open", trans_cb, NULL));
	tl = intercept_register_transfn("netbsd", "open", 0);
	intercept_register_translation("netbsd", "open", 1, &ic_oflags);
	alias = systrace_new_alias("netbsd", "open", "netbsd", "fswrite");
	systrace_alias_add_trans(alias, tl);

	/* 9: link */
	X(intercept_register_sccb("netbsd", "link", trans_cb, NULL));
	intercept_register_transfn("netbsd", "link", 0);
	intercept_register_transfn("netbsd", "link", 1);

	/* 10: unlink [fswrite] */
	X(intercept_register_sccb("netbsd", "unlink", trans_cb, NULL));
	tl = intercept_register_translation("netbsd", "unlink", 0,
	    &ic_translate_unlinkname);
	alias = systrace_new_alias("netbsd", "unlink", "netbsd", "fswrite");
	systrace_alias_add_trans(alias, tl);

	/* 12: chdir */
	X(intercept_register_sccb("netbsd", "chdir", trans_cb, NULL));
	intercept_register_transfn("netbsd", "chdir", 0);

	/* 15: chmod */
	X(intercept_register_sccb("netbsd", "chmod", trans_cb, NULL));
	intercept_register_transfn("netbsd", "chmod", 0);
	intercept_register_translation("netbsd", "chmod", 1, &ic_modeflags);

	/* 16: chown */
	X(intercept_register_sccb("netbsd", "chown", trans_cb, NULL));
	intercept_register_transfn("netbsd", "chown", 0);
	intercept_register_translation("netbsd", "chown", 1, &ic_uidt);
	intercept_register_translation("netbsd", "chown", 2, &ic_gidt);

	/* 23: setuid */
	X(intercept_register_sccb("netbsd", "setuid", trans_cb, NULL));
	intercept_register_translation("netbsd", "setuid", 0, &ic_uidt);
	intercept_register_translation("netbsd", "setuid", 0, &ic_uname);

	/* 33: access [fsread] */
	X(intercept_register_sccb("netbsd", "access", trans_cb, NULL));
	tl = intercept_register_transfn("netbsd", "access", 0);
	alias = systrace_new_alias("netbsd", "access", "netbsd", "fsread");
	systrace_alias_add_trans(alias, tl);

	/* 37: kill */
 	X(intercept_register_sccb("netbsd", "kill", trans_cb, NULL));
 	intercept_register_translation("netbsd", "kill", 0, &ic_pidname);
 	intercept_register_translation("netbsd", "kill", 1, &ic_signame);

	/* 57: symlink */
	X(intercept_register_sccb("netbsd", "symlink", trans_cb, NULL));
	intercept_register_transstring("netbsd", "symlink", 0);
	intercept_register_transfn("netbsd", "symlink", 1);

	/* 58: readlink [fsread] */
	X(intercept_register_sccb("netbsd", "readlink", trans_cb, NULL));
	tl = intercept_register_translation("netbsd", "readlink", 0,
	    &ic_translate_unlinkname);
	alias = systrace_new_alias("netbsd", "readlink", "netbsd", "fsread");
	systrace_alias_add_trans(alias, tl);

	/* 59: execve */
	X(intercept_register_sccb("netbsd", "execve", trans_cb, NULL));
	intercept_register_transfn("netbsd", "execve", 0);
	intercept_register_translation("netbsd", "execve", 1, &ic_trargv);

	/* 61: chroot */
	X(intercept_register_sccb("netbsd", "chroot", trans_cb, NULL));
	intercept_register_transfn("netbsd", "chroot", 0);

	/* 92: fcntl */
 	X(intercept_register_sccb("netbsd", "fcntl", trans_cb, NULL));
 	intercept_register_translation("netbsd", "fcntl", 1, &ic_fcntlcmd);

	/* 97: socket */
 	X(intercept_register_sccb("netbsd", "socket", trans_cb, NULL));
 	intercept_register_translation("netbsd", "socket", 0, &ic_sockdom);
 	intercept_register_translation("netbsd", "socket", 1, &ic_socktype);

	/* 98: connect */
	X(intercept_register_sccb("netbsd", "connect", trans_cb, NULL));
	intercept_register_translation("netbsd", "connect", 1,
	    &ic_translate_connect);

	/* 104: bind */
	X(intercept_register_sccb("netbsd", "bind", trans_cb, NULL));
	intercept_register_translation("netbsd", "bind", 1,
	    &ic_translate_connect);

	/* 123: fchown */
	X(intercept_register_sccb("netbsd", "fchown", trans_cb, NULL));
	intercept_register_translation("netbsd", "fchown", 0, &ic_fdt);
	intercept_register_translation("netbsd", "fchown", 1, &ic_uidt);
	intercept_register_translation("netbsd", "fchown", 2, &ic_gidt);

	/* 124: fchmod */
	X(intercept_register_sccb("netbsd", "fchmod", trans_cb, NULL));
	intercept_register_translation("netbsd", "fchmod", 0, &ic_fdt);
	intercept_register_translation("netbsd", "fchmod", 1, &ic_modeflags);

	/* 128: rename */
	X(intercept_register_sccb("netbsd", "rename", trans_cb, NULL));
	intercept_register_translation("netbsd", "rename", 0,
	    &ic_translate_unlinkname);
	intercept_register_transfn("netbsd", "rename", 1);

	/* 133: sendto */
	X(intercept_register_sccb("netbsd", "sendto", trans_cb, NULL));
	intercept_register_translation("netbsd", "sendto", 4,
	    &ic_translate_connect);

	/* 136: mkdir [fswrite] */
	X(intercept_register_sccb("netbsd", "mkdir", trans_cb, NULL));
	tl = intercept_register_translation("netbsd", "mkdir", 0,
	    &ic_translate_unlinkname);
	alias = systrace_new_alias("netbsd", "mkdir", "netbsd", "fswrite");
	systrace_alias_add_trans(alias, tl);

	/* 137: rmdir [fswrite] */
	X(intercept_register_sccb("netbsd", "rmdir", trans_cb, NULL));
	tl = intercept_register_transfn("netbsd", "rmdir", 0);
	alias = systrace_new_alias("netbsd", "rmdir", "netbsd", "fswrite");
	systrace_alias_add_trans(alias, tl);

	/* 181: setgid */
	X(intercept_register_sccb("netbsd", "setgid", trans_cb, NULL));
	intercept_register_translation("netbsd", "setgid", 0, &ic_gidt);

	/* 182: setegid */
	X(intercept_register_sccb("netbsd", "setegid", trans_cb, NULL));
	intercept_register_translation("netbsd", "setegid", 0, &ic_gidt);

	/* 183: seteuid */
	X(intercept_register_sccb("netbsd", "seteuid", trans_cb, NULL));
	intercept_register_translation("netbsd", "seteuid", 0, &ic_uidt);
	intercept_register_translation("netbsd", "seteuid", 0, &ic_uname);

	/* 270: __posix_rename */
	X(intercept_register_sccb("netbsd", "__posix_rename", trans_cb, NULL));
	intercept_register_translation("netbsd", "__posix_rename", 0,
	    &ic_translate_unlinkname);
	intercept_register_transfn("netbsd", "__posix_rename", 1);

	/* 278: __stat13 [fsread] */
	X(intercept_register_sccb("netbsd", "__stat13", trans_cb, NULL));
	tl = intercept_register_transfn("netbsd", "__stat13", 0);
	alias = systrace_new_alias("netbsd", "__stat13", "netbsd", "fsread");
	systrace_alias_add_trans(alias, tl);

	/* 280: __lstat13 [fsread] */
	X(intercept_register_sccb("netbsd", "__lstat13", trans_cb, NULL));
	tl = intercept_register_translation("netbsd", "__lstat13", 0,
	    &ic_translate_unlinkname);
	alias = systrace_new_alias("netbsd", "__lstat13", "netbsd", "fsread");
	systrace_alias_add_trans(alias, tl);

	/* 283: __posix_chown */
	X(intercept_register_sccb("netbsd", "__posix_chown", trans_cb, NULL));
	intercept_register_transfn("netbsd", "__posix_chown", 0);
	intercept_register_translation("netbsd", "__posix_chown", 1, &ic_uidt);
	intercept_register_translation("netbsd", "__posix_chown", 2, &ic_gidt);

	/* 284: __posix_fchown */
	X(intercept_register_sccb("netbsd", "__posix_fchown", trans_cb, NULL));
	intercept_register_translation("netbsd", "__posix_fchown", 0, &ic_fdt);
	intercept_register_translation("netbsd", "__posix_fchown", 1, &ic_uidt);
	intercept_register_translation("netbsd", "__posix_fchown", 2, &ic_gidt);
#else
	X(intercept_register_gencb(gen_cb, NULL));

	/* open */
	X(intercept_register_sccb("native", "open", trans_cb, NULL));
	tl = intercept_register_transfn("native", "open", 0);
	intercept_register_translation("native", "open", 1, &ic_oflags);
	alias = systrace_new_alias("native", "open", "native", "fswrite");
	systrace_alias_add_trans(alias, tl);

	/* link */
	X(intercept_register_sccb("native", "link", trans_cb, NULL));
	intercept_register_transfn("native", "link", 0);
	intercept_register_transfn("native", "link", 1);

	/* unlink [fswrite] */
	X(intercept_register_sccb("native", "unlink", trans_cb, NULL));
	tl = intercept_register_translation("native", "unlink", 0,
	    &ic_translate_unlinkname);
	alias = systrace_new_alias("native", "unlink", "native", "fswrite");
	systrace_alias_add_trans(alias, tl);

	/* chdir */
	X(intercept_register_sccb("native", "chdir", trans_cb, NULL));
	intercept_register_transfn("native", "chdir", 0);

	/* chmod */
	X(intercept_register_sccb("native", "chmod", trans_cb, NULL));
	intercept_register_transfn("native", "chmod", 0);
	intercept_register_translation("native", "chmod", 1, &ic_modeflags);

	/* chown */
	X(intercept_register_sccb("native", "chown", trans_cb, NULL));
	intercept_register_transfn("native", "chown", 0);
	intercept_register_translation("native", "chown", 1, &ic_uidt);
	intercept_register_translation("native", "chown", 2, &ic_gidt);

	/* setuid */
	X(intercept_register_sccb("native", "setuid", trans_cb, NULL));
	intercept_register_translation("native", "setuid", 0, &ic_uidt);
	intercept_register_translation("native", "setuid", 0, &ic_uname);

	/* access [fsread] */
	X(intercept_register_sccb("native", "access", trans_cb, NULL));
	tl = intercept_register_transfn("native", "access", 0);
	alias = systrace_new_alias("native", "access", "native", "fsread");
	systrace_alias_add_trans(alias, tl);

	/* kill */
 	X(intercept_register_sccb("native", "kill", trans_cb, NULL));
 	intercept_register_translation("native", "kill", 0, &ic_pidname);
 	intercept_register_translation("native", "kill", 1, &ic_signame);

	/* stat [fsread] */
	X(intercept_register_sccb("native", "stat", trans_cb, NULL));
	tl = intercept_register_transfn("native", "stat", 0);
	alias = systrace_new_alias("native", "stat", "native", "fsread");
	systrace_alias_add_trans(alias, tl);

	/* lstat [fsread] */
	X(intercept_register_sccb("native", "lstat", trans_cb, NULL));
	tl = intercept_register_translation("native", "lstat", 0,
	    &ic_translate_unlinkname);
	alias = systrace_new_alias("native", "lstat", "native", "fsread");
	systrace_alias_add_trans(alias, tl);

	/* symlink */
	X(intercept_register_sccb("native", "symlink", trans_cb, NULL));
	intercept_register_transstring("native", "symlink", 0);
	intercept_register_transfn("native", "symlink", 1);

	/* readlink [fsread] */
	X(intercept_register_sccb("native", "readlink", trans_cb, NULL));
	tl = intercept_register_translation("native", "readlink", 0,
	    &ic_translate_unlinkname);
	alias = systrace_new_alias("native", "readlink", "native", "fsread");
	systrace_alias_add_trans(alias, tl);

	/* execve */
	X(intercept_register_sccb("native", "execve", trans_cb, NULL));
	intercept_register_transfn("native", "execve", 0);
	intercept_register_translation("native", "execve", 1, &ic_trargv);

	/* chroot */
	X(intercept_register_sccb("native", "chroot", trans_cb, NULL));
	intercept_register_transfn("native", "chroot", 0);

	/* fcntl */
 	X(intercept_register_sccb("native", "fcntl", trans_cb, NULL));
 	intercept_register_translation("native", "fcntl", 1, &ic_fcntlcmd);

	/* socket */
 	X(intercept_register_sccb("native", "socket", trans_cb, NULL));
 	intercept_register_translation("native", "socket", 0, &ic_sockdom);
 	intercept_register_translation("native", "socket", 1, &ic_socktype);

	/* connect */
	X(intercept_register_sccb("native", "connect", trans_cb, NULL));
	intercept_register_translation("native", "connect", 1,
	    &ic_translate_connect);

	/* bind */
	X(intercept_register_sccb("native", "bind", trans_cb, NULL));
	intercept_register_translation("native", "bind", 1,
	    &ic_translate_connect);

	/* fchown */
	X(intercept_register_sccb("native", "fchown", trans_cb, NULL));
	intercept_register_translation("native", "fchown", 0, &ic_fdt);
	intercept_register_translation("native", "fchown", 1, &ic_uidt);
	intercept_register_translation("native", "fchown", 2, &ic_gidt);

	/* fchmod */
	X(intercept_register_sccb("native", "fchmod", trans_cb, NULL));
	intercept_register_translation("native", "fchmod", 0, &ic_fdt);
	intercept_register_translation("native", "fchmod", 1, &ic_modeflags);

	/* rename */
	X(intercept_register_sccb("native", "rename", trans_cb, NULL));
	intercept_register_translation("native", "rename", 0,
	    &ic_translate_unlinkname);
	intercept_register_transfn("native", "rename", 1);

	/* sendto */
	X(intercept_register_sccb("native", "sendto", trans_cb, NULL));
	intercept_register_translation("native", "sendto", 4,
	    &ic_translate_connect);

	/* mkdir [fswrite] */
	X(intercept_register_sccb("native", "mkdir", trans_cb, NULL));
	tl = intercept_register_translation("native", "mkdir", 0,
	    &ic_translate_unlinkname);
	alias = systrace_new_alias("native", "mkdir", "native", "fswrite");
	systrace_alias_add_trans(alias, tl);

	/* rmdir [fswrite] */
	X(intercept_register_sccb("native", "rmdir", trans_cb, NULL));
	tl = intercept_register_transfn("native", "rmdir", 0);
	alias = systrace_new_alias("native", "rmdir", "native", "fswrite");
	systrace_alias_add_trans(alias, tl);

	/* setgid */
	X(intercept_register_sccb("native", "setgid", trans_cb, NULL));
	intercept_register_translation("native", "setgid", 0, &ic_gidt);

	/* setegid */
	X(intercept_register_sccb("native", "setegid", trans_cb, NULL));
	intercept_register_translation("native", "setegid", 0, &ic_gidt);

	/* seteuid */
	X(intercept_register_sccb("native", "seteuid", trans_cb, NULL));
	intercept_register_translation("native", "seteuid", 0, &ic_uidt);
	intercept_register_translation("native", "seteuid", 0, &ic_uname);
#endif

#if !(defined(__NetBSD__) && !defined(HAVE_LINUX_FCNTL_H))
	/* 5: open [fswrite] */
	X(intercept_register_sccb("linux", "open", trans_cb, NULL));
	tl = intercept_register_translink("linux", "open", 0);
	intercept_register_translation("linux", "open", 1, &ic_linux_oflags);
	alias = systrace_new_alias("linux", "open", "linux", "fswrite");
	systrace_alias_add_trans(alias, tl);

	/* 9: link */
	X(intercept_register_sccb("linux", "link", trans_cb, NULL));
	intercept_register_translink("linux", "link", 0);
	intercept_register_translink("linux", "link", 1);

	/* 10: unlink [fswrite] */
	X(intercept_register_sccb("linux", "unlink", trans_cb, NULL));
	tl = intercept_register_translink("linux", "unlink", 0);
	alias = systrace_new_alias("linux", "unlink", "linux", "fswrite");
	systrace_alias_add_trans(alias, tl);

	/* 11: execve */
	X(intercept_register_sccb("linux", "execve", trans_cb, NULL));
	intercept_register_translink("linux", "execve", 0);

	/* 15: chmod */
	X(intercept_register_sccb("linux", "chmod", trans_cb, NULL));
	intercept_register_translink("linux", "chmod", 0);
	intercept_register_translation("linux", "chmod", 1, &ic_modeflags);

	/* 33: access [fsread] */
	X(intercept_register_sccb("linux", "access", trans_cb, NULL));
	tl = intercept_register_translink("linux", "access", 0);
	alias = systrace_new_alias("linux", "access", "linux", "fsread");
	systrace_alias_add_trans(alias, tl);

	/* 38: rename */
	X(intercept_register_sccb("linux", "rename", trans_cb, NULL));
	intercept_register_translink("linux", "rename", 0);
	intercept_register_translink("linux", "rename", 1);

	/* 39: mkdir [fswrite] */
	X(intercept_register_sccb("linux", "mkdir", trans_cb, NULL));
	tl = intercept_register_translink("linux", "mkdir", 0);
	alias = systrace_new_alias("linux", "mkdir", "linux", "fswrite");
	systrace_alias_add_trans(alias, tl);

	/* 40: rmdir [fswrite] */
	X(intercept_register_sccb("linux", "rmdir", trans_cb, NULL));
	tl = intercept_register_translink("linux", "rmdir", 0);
	alias = systrace_new_alias("linux", "rmdir", "linux", "fswrite");
	systrace_alias_add_trans(alias, tl);

	/* 83: symlink */
	X(intercept_register_sccb("linux", "symlink", trans_cb, NULL));
	intercept_register_transstring("linux", "symlink", 0);
	intercept_register_translink("linux", "symlink", 1);

	/* 85: readlink [fsread] */
	X(intercept_register_sccb("linux", "readlink", trans_cb, NULL));
	tl = intercept_register_translink("linux", "readlink", 0);
	alias = systrace_new_alias("linux", "readlink", "linux", "fsread");
	systrace_alias_add_trans(alias, tl);

	/* 106: stat [fsread] */
	X(intercept_register_sccb("linux", "stat", trans_cb, NULL));
	tl = intercept_register_translink("linux", "stat", 0);
	alias = systrace_new_alias("linux", "stat", "linux", "fsread");
	systrace_alias_add_trans(alias, tl);

	/* 107: lstat */
	X(intercept_register_sccb("linux", "lstat", trans_cb, NULL));
	tl = intercept_register_translink("linux", "lstat", 0);
	alias = systrace_new_alias("linux", "lstat", "linux", "fsread");
	systrace_alias_add_trans(alias, tl);
#endif

	X(intercept_register_execcb(execres_cb, NULL));
	X(intercept_register_pfreecb(policyfree_cb, NULL));
}
