#	$NetBSD: Makefile,v 1.4 2000/12/25 22:39:21 itojun Exp $

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
	${PIC} ${.ALLSRC} | ${EQN} | ${ROFF} >${.TARGET}

.include <bsd.doc.mk>
