#	$NetBSD: Makefile,v 1.3 2012/06/22 20:32:34 abs Exp $

BINDIR=	/usr/sbin
PROG=	cron
SRCS=	cron.c database.c do_command.c entry.c env.c job.c \
	misc.c popen.c pw_dup.c user.c
CPPFLAGS+=-I${.CURDIR} -DLOGIN_CAP
LDADD+=-lutil
MAN=	cron.8

CWARNFLAGS.clang+=	-Wno-string-plus-int

.include <bsd.prog.mk>
