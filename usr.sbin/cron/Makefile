#	$NetBSD: Makefile,v 1.13 2008/08/29 00:02:25 gmcgarry Exp $

PROG=	cron
SRCS=	cron.c database.c do_command.c entry.c env.c job.c \
	misc.c popen.c user.c
CPPFLAGS+=-I${.CURDIR} -DLOGIN_CAP
LDADD+=-lutil
MAN=	cron.8

WARNS=  3

.include <bsd.prog.mk>

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.misc.c+=	-Wno-pointer-sign
.endif
