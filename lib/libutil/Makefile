#	$NetBSD: Makefile,v 1.29 2001/04/06 14:31:14 wiz Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

LIB=	util
CPPFLAGS+=-DLIBC_SCCS
SRCS=	fparseln.c getbootfile.c getmaxpartitions.c getrawpartition.c \
	login.c login_cap.c login_tty.c \
	logout.c logwtmp.c opendisk.c passwd.c pw_scan.c pidfile.c pidlock.c \
	pty.c securepath.c ttyaction.c ttymsg.c

MAN=	fparseln.3 getbootfile.3 getmaxpartitions.3 getrawpartition.3 \
	login.3 login_cap.3 opendisk.3 \
	openpty.3 pidfile.3 pidlock.3 pw_getconf.3 pw_init.3 pw_lock.3 \
	secure_path.3 ttyaction.3 ttymsg.3

.PATH:	${.CURDIR}/../libc/gen

MLINKS+=login.3 logout.3
MLINKS+=login.3 logwtmp.3
MLINKS+=login_cap.3 login_getclass.3
MLINKS+=login_cap.3 login_getcapbool.3
MLINKS+=login_cap.3 login_getcapnum.3
MLINKS+=login_cap.3 login_getcapsize.3
MLINKS+=login_cap.3 login_getcapstr.3
MLINKS+=login_cap.3 login_getcaptime.3
MLINKS+=login_cap.3 login_close.3
MLINKS+=login_cap.3 setclasscontext.3
MLINKS+=login_cap.3 setusercontext.3
MLINKS+=openpty.3 login_tty.3
MLINKS+=openpty.3 forkpty.3
MLINKS+=pw_init.3 pw_edit.3
MLINKS+=pw_init.3 pw_prompt.3
MLINKS+=pw_init.3 pw_copy.3
MLINKS+=pw_init.3 pw_scan.3
MLINKS+=pw_init.3 pw_error.3
MLINKS+=pw_lock.3 pw_mkdb.3
MLINKS+=pw_lock.3 pw_abort.3
MLINKS+=pw_lock.3 pw_getprefix.3
MLINKS+=pw_lock.3 pw_setprefix.3
MLINKS+=pidlock.3 ttylock.3
MLINKS+=pidlock.3 ttyunlock.3

.include <bsd.lib.mk>
