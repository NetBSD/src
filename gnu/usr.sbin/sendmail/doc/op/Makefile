#	$NetBSD: Makefile,v 1.2 2000/04/28 06:20:02 itojun Exp $

DIST=	${.CURDIR}/../../../../dist
.PATH: ${DIST}/sendmail/doc/op

DIR=	smm/08.sendmailop
SRCS=	op.me
MACROS=	-me
RM=	rm -f

all: op.ps

op.ps: ${SRCS}
	${RM} ${.TARGET}
	${PIC} ${.ALLSRC} | ${ROFF} >${.TARGET}

.include <bsd.doc.mk>
