#	$NetBSD: Makefile,v 1.1.1.1 2000/02/08 13:45:36 tron Exp $

DIST=	${.CURDIR}/../../../../dist
.PATH: ${DIST}/sendmail/doc/op

DIR=	smm/08.sendmailop
SRCS=	op.me
MACROS=	-me
RM=	rm -f

all: op.ps

op.ps: ${SRCS}
	${RM} ${.TARGET}
	${PIC} ${SRCS} | ${ROFF} >${.TARGET}

.include <bsd.doc.mk>
