#	$NetBSD: Makefile,v 1.22 1998/02/22 12:16:03 christos Exp $

PROG=	crontab
SRCS=	crontab.c misc.c entry.c env.c
CPPFLAGS+=-I${.CURDIR} -I${.CURDIR}/../../usr.sbin/cron -DDEBUGGING=1
CPPFLAGS+=-DUSE_UTIMES
BINOWN =root
BINMODE=4555
MAN=	crontab.1 crontab.5

.PATH: ${.CURDIR}/../../usr.sbin/cron

.include <bsd.prog.mk>
