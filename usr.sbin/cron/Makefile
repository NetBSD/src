#	$NetBSD: Makefile,v 1.7 1997/10/18 06:42:37 lukem Exp $

WARNS?=	0
PROG=	cron
SRCS=	cron.c database.c do_command.c entry.c env.c job.c \
	misc.c popen.c user.c
CFLAGS+=-I${.CURDIR}
MAN=	cron.8

.include <bsd.prog.mk>
