#	$NetBSD: Makefile,v 1.6 2003/01/05 12:28:49 pooka Exp $

.include <bsd.own.mk>

DIST=	${NETBSDSRCDIR}/gnu/dist
.PATH: ${DIST}/sendmail/doc/op

DIR=	smm/08.sendmailop
SRCS=	op.me
MACROS=	-me
RM=	rm -f

CLEANFILES+=	op.ps

all: op.ps

op.ps: ${SRCS}
	${RM} ${.TARGET}
	${PIC} ${.ALLSRC} | ${EQN} | ${ROFF} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
