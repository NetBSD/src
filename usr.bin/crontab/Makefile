#	$NetBSD: Makefile,v 1.23 2002/09/18 14:00:36 lukem Exp $

.include <bsd.own.mk>

PROG=	crontab
SRCS=	crontab.c misc.c entry.c env.c
CPPFLAGS+=-I${.CURDIR} -I${NETBSDSRCDIR}/usr.sbin/cron -DDEBUGGING=1
CPPFLAGS+=-DUSE_UTIMES
BINOWN =root
BINMODE=4555
MAN=	crontab.1 crontab.5

.PATH: ${NETBSDSRCDIR}/usr.sbin/cron

.include <bsd.prog.mk>
