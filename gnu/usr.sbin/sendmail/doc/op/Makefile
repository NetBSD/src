#	$NetBSD: Makefile,v 1.5 2002/09/19 03:09:55 lukem Exp $

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
	${PIC} ${.ALLSRC} | ${EQN} | ${ROFF} >${.TARGET}

.include <bsd.doc.mk>
