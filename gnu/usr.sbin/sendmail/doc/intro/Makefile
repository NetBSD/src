#	$NetBSD: Makefile,v 1.4 2003/01/05 12:28:48 pooka Exp $

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
	${PIC} ${.ALLSRC} | ${ROFF} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
