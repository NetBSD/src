#	$NetBSD: Makefile,v 1.11 2005/03/16 02:53:55 xtraeme Exp $

PROG=	cron
SRCS=	cron.c database.c do_command.c entry.c env.c job.c \
	misc.c popen.c user.c
CPPFLAGS+=-I${.CURDIR} -DLOGIN_CAP
LDADD+=-lutil
MAN=	cron.8

WARNS=  3

.include <bsd.prog.mk>
