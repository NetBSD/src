#	$NetBSD: Makefile,v 1.14.4.1 2011/06/23 14:20:46 cherry Exp $

PROG=	cron
SRCS=	cron.c database.c do_command.c entry.c env.c job.c \
	misc.c popen.c user.c
CPPFLAGS+=-I${.CURDIR} -DLOGIN_CAP
LDADD+=-lutil
MAN=	cron.8

.include <bsd.prog.mk>

.if defined(HAVE_GCC) || defined(HAVE_PCC)
COPTS.misc.c+=	-Wno-pointer-sign
.endif
