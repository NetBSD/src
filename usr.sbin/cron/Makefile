#	$NetBSD: Makefile,v 1.8 1997/10/23 06:34:28 lukem Exp $

WARNS?=	0
PROG=	cron
SRCS=	cron.c database.c do_command.c entry.c env.c job.c \
	misc.c popen.c user.c
CPPFLAGS+=-I${.CURDIR}
MAN=	cron.8

.include <bsd.prog.mk>
