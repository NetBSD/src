#	$NetBSD: Makefile,v 1.3.2.1 2001/03/09 18:01:25 he Exp $

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
