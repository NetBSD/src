#	$NetBSD: Makefile,v 1.10 2003/03/14 21:56:07 christos Exp $

PROG=	cron
SRCS=	cron.c database.c do_command.c entry.c env.c job.c \
	misc.c popen.c user.c
CPPFLAGS+=-I${.CURDIR} -DLOGIN_CAP
LDADD+=-lutil
MAN=	cron.8

.include <bsd.prog.mk>
