#	$Id: Makefile,v 1.12 1994/12/22 12:29:38 cgd Exp $

.include <bsd.own.mk>

PROG=	crontab
SRCS=	crontab.c misc.c entry.c env.c
CFLAGS+=-I${.CURDIR} -I${.CURDIR}/../../usr.sbin/cron -DDEBUGGING=1 
BINOWN =root
BINMODE=4111
MAN=	crontab.1 crontab.5

.PATH: ${.CURDIR}/../../usr.sbin/cron

.include <bsd.prog.mk>
