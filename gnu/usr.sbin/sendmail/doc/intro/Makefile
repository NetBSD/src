#	$NetBSD: Makefile,v 1.3 2002/09/19 03:09:55 lukem Exp $

.include <bsd.own.mk>

DIST=	${NETBSDSRCDIR}/gnu/dist
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
