#	$NetBSD: Makefile,v 1.2 2000/04/28 06:20:01 itojun Exp $

DIST=	${.CURDIR}/../../../../dist
.PATH: ${DIST}/sendmail/doc/intro

DIR=	smm/09.sendmail
SRCS=	intro.me
MACROS=	-me
RM=	rm -f

all: intro.ps

intro.ps: ${SRCS}
	${RM} ${.TARGET}
	${PIC} ${.ALLSRC} | ${ROFF} >${.TARGET}

.include <bsd.doc.mk>
