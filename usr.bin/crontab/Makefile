#	$NetBSD: Makefile,v 1.19 1997/10/19 13:00:58 lukem Exp $

WARNS=	0
PROG=	crontab
SRCS=	crontab.c misc.c entry.c env.c
CFLAGS+=-I${.CURDIR} -I${.CURDIR}/../../usr.sbin/cron -DDEBUGGING=1 
BINOWN =root
BINMODE=4555
MAN=	crontab.1 crontab.5

.PATH: ${.CURDIR}/../../usr.sbin/cron

.include <bsd.prog.mk>
