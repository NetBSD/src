#	$NetBSD: Makefile,v 1.21 1999/01/15 13:31:22 bouyer Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

LIB=	util
CPPFLAGS+=-DLIBC_SCCS
SRCS=	fparseln.c getmaxpartitions.c getrawpartition.c login.c login_tty.c \
	logout.c logwtmp.c opendisk.c passwd.c pw_scan.c pidlock.c pty.c \
	ttyaction.c ttymsg.c 

MAN=	fparseln.3 getmaxpartitions.3 getrawpartition.3 login.3 opendisk.3 \
	openpty.3 pidlock.3 pw_init.3 pw_lock.3 ttyaction.3 ttymsg.3 

.PATH:	${.CURDIR}/../libc/gen

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
MLINKS+=pidlock.3 ttylock.3
MLINKS+=pidlock.3 ttyunlock.3

.include <bsd.lib.mk>
