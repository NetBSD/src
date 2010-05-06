#	$NetBSD: Makefile,v 1.1 2010/05/06 18:58:45 christos Exp $

BINDIR=	/usr/sbin
PROG=	cron
SRCS=	cron.c database.c do_command.c entry.c env.c job.c \
	misc.c popen.c pw_dup.c user.c
CPPFLAGS+=-I${.CURDIR} -DLOGIN_CAP
LDADD+=-lutil
MAN=	cron.8

.include <bsd.prog.mk>
