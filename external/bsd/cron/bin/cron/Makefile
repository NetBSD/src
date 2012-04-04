#	$NetBSD: Makefile,v 1.2 2012/04/04 10:59:44 joerg Exp $

BINDIR=	/usr/sbin
PROG=	cron
SRCS=	cron.c database.c do_command.c entry.c env.c job.c \
	misc.c popen.c pw_dup.c user.c
CPPFLAGS+=-I${.CURDIR} -DLOGIN_CAP
LDADD+=-lutil
MAN=	cron.8

CWARNFLAGS+=	-Wno-string-plus-int

.include <bsd.prog.mk>
