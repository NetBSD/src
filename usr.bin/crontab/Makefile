#	$Id: Makefile,v 1.11 1994/12/20 15:16:18 cgd Exp $

.include <bsd.own.mk>

PROG=	crontab
SRCS=	crontab.c misc.c entry.c env.c
CFLAGS+=-I${.CURDIR} -I${.CURDIR}/../../usr.sbin/cron -DDEBUGGING=1 
BINOWN =root
BINMODE=4111
MAN1=
MAN5=
CLEANFILES=crontab.1.0 crontab.5.0

.if !defined(NOMAN)
all:	crontab.1.0 crontab.5.0

crontab.1.0:	crontab.1
	nroff -mandoc ${.ALLSRC} > ${.TARGET}

crontab.5.0:	crontab.5
	nroff -mandoc ${.ALLSRC} > ${.TARGET}

maninstall:
	${MINSTALL} crontab.1.0 ${DESTDIR}${MANDIR}1/crontab.0
	${MINSTALL} crontab.5.0 ${DESTDIR}${MANDIR}5/crontab.0
.endif

.include <bsd.prog.mk>
.PATH: ${.CURDIR}/../../usr.sbin/cron
