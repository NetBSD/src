#	$NetBSD: Makefile,v 1.7.2.1 1997/11/08 15:11:31 lukem Exp $

WARNS?=	0
PROG=	cron
SRCS=	cron.c database.c do_command.c entry.c env.c job.c \
	misc.c popen.c user.c
CPPFLAGS+=-I${.CURDIR}
MAN=	cron.8

.include <bsd.prog.mk>
