#	$NetBSD: Makefile,v 1.12 2006/05/11 23:16:30 mrg Exp $

PROG=	cron
SRCS=	cron.c database.c do_command.c entry.c env.c job.c \
	misc.c popen.c user.c
CPPFLAGS+=-I${.CURDIR} -DLOGIN_CAP
LDADD+=-lutil
MAN=	cron.8

WARNS=  3

.include <bsd.prog.mk>

.if ${HAVE_GCC} == 4
COPTS.misc.c+=	-Wno-pointer-sign
.endif
