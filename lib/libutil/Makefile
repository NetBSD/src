#	$NetBSD: Makefile,v 1.51 2006/10/20 17:40:39 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

USE_SHLIBDIR=	yes

.include <bsd.own.mk>

WARNS=3
LIB=	util
CPPFLAGS+=-DLIBC_SCCS
SRCS=	efun.c getbootfile.c getlabelsector.c getmaxpartitions.c \
	getmntopts.c getrawpartition.c \
	disklabel_dkcksum.c disklabel_scan.c \
	if_media.c \
	login.c loginx.c login_cap.c login_tty.c logout.c logoutx.c \
	logwtmp.c logwtmpx.c opendisk.c \
	passwd.c pw_scan.c pw_policy.c pidfile.c pidlock.c pty.c \
	secure_path.c snprintb.c sockaddr_snprintf.c ttyaction.c ttymsg.c

MAN=	efun.3 getbootfile.3 getlabelsector.3 getmaxpartitions.3 \
	getmntopts.3 \
	getrawpartition.3 \
	login.3 login_cap.3 loginx.3 \
	disklabel_dkcksum.3 disklabel_scan.3 \
	opendisk.3 openpty.3 pidfile.3 pidlock.3 \
	pw_getconf.3 pw_init.3 pw_lock.3 pw_policy.3 secure_path.3 \
	snprintb.3 sockaddr_snprintf.3 ttyaction.3 \
	ttymsg.3 util.3

.PATH:	${NETBSDSRCDIR}/lib/libc/gen

MLINKS+=getlabelsector.3 getlabeloffset.3
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
MLINKS+=loginx.3 logoutx.3 loginx.3 logwtmpx.3
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
MLINKS+=efun.3 esetfunc.3
MLINKS+=efun.3 easprintf.3
MLINKS+=efun.3 estrlcpy.3
MLINKS+=efun.3 estrlcat.3
MLINKS+=efun.3 estrdup.3
MLINKS+=efun.3 emalloc.3
MLINKS+=efun.3 ecalloc.3
MLINKS+=efun.3 erealloc.3
MLINKS+=efun.3 efopen.3
MLINKS+=efun.3 evasprintf.3

.include <bsd.lib.mk>
