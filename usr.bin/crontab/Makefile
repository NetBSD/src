#	$NetBSD: Makefile,v 1.16 1997/05/06 20:45:31 gwr Exp $

PROG=	crontab
SRCS=	crontab.c misc.c entry.c env.c
CFLAGS+=-I${.CURDIR} -I${.CURDIR}/../../usr.sbin/cron -DDEBUGGING=1 
BINOWN =root
BINMODE=4555
MAN=	crontab.1 crontab.5

.PATH.c: ${.CURDIR}/../../usr.sbin/cron

.include <bsd.prog.mk>
