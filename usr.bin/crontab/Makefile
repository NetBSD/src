#	$NetBSD: Makefile,v 1.20.2.1 1997/11/08 10:36:47 lukem Exp $

WARNS=	0
PROG=	crontab
SRCS=	crontab.c misc.c entry.c env.c
CPPFLAGS+=-I${.CURDIR} -I${.CURDIR}/../../usr.sbin/cron -DDEBUGGING=1
CPPFLAGS+=-DUSE_UTIMES
BINOWN =root
BINMODE=4555
MAN=	crontab.1 crontab.5

.PATH: ${.CURDIR}/../../usr.sbin/cron

.include <bsd.prog.mk>
