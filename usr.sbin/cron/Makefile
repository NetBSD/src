#	$NetBSD: Makefile,v 1.6 1997/03/13 06:19:07 mikel Exp $

PROG=	cron
SRCS=	cron.c database.c do_command.c entry.c env.c job.c \
	misc.c popen.c user.c
CFLAGS+=-I${.CURDIR}
MAN=	cron.8

.include <bsd.prog.mk>
