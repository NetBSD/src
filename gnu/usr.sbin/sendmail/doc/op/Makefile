#	$NetBSD: Makefile,v 1.3 2000/05/03 10:50:58 itojun Exp $

DIST=	${.CURDIR}/../../../../dist
.PATH: ${DIST}/sendmail/doc/op

DIR=	smm/08.sendmailop
SRCS=	op.me
MACROS=	-me
RM=	rm -f

CLEANFILES+=	op.ps

all: op.ps

op.ps: ${SRCS}
	${RM} ${.TARGET}
	${PIC} ${.ALLSRC} | ${ROFF} >${.TARGET}

.include <bsd.doc.mk>
