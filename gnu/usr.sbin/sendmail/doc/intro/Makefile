#	$NetBSD: Makefile,v 1.1.1.1 2000/02/08 13:45:36 tron Exp $

DIST=	${.CURDIR}/../../../../dist
.PATH: ${DIST}/sendmail/doc/intro

DIR=	smm/09.sendmail
SRCS=	intro.me
MACROS=	-me
RM=	rm -f

all: intro.ps

intro.ps: ${SRCS}
	${RM} ${.TARGET}
	${PIC} ${SRCS} | ${ROFF} >${.TARGET}

.include <bsd.doc.mk>
