#	$NetBSD: Makefile,v 1.7 1996/05/15 21:42:23 jtc Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

LIB=	util
CFLAGS+=-DLIBC_SCCS
SRCS=	login.c login_tty.c logout.c logwtmp.c passwd.c pty.c

MAN=	login.3 openpty.3 pw_init.3 pw_lock.3

MLINKS+=login.3 logout.3
MLINKS+=login.3 logwtmp.3
MLINKS+=openpty.3 login_tty.3
MLINKS+=openpty.3 forkpty.3
MLINKS+=pw_init.3 pw_edit.3
MLINKS+=pw_init.3 pw_prompt.3
MLINKS+=pw_init.3 pw_copy.3
MLINKS+=pw_init.3 pw_scan.3
MLINKS+=pw_init.3 pw_error.3
MLINKS+=pw_lock.3 pw_mkdb.3
MLINKS+=pw_lock.3 pw_abort.3

.include <bsd.lib.mk>
