#	$NetBSD: Makefile,v 1.9 1998/01/31 14:40:13 christos Exp $

PROG=	cron
SRCS=	cron.c database.c do_command.c entry.c env.c job.c \
	misc.c popen.c user.c
CPPFLAGS+=-I${.CURDIR}
MAN=	cron.8

.include <bsd.prog.mk>
