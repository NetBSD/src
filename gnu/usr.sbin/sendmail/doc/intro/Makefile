#	$NetBSD: Makefile,v 1.6 2003/10/22 06:04:03 lukem Exp $

.include <bsd.own.mk>

DIST=	${NETBSDSRCDIR}/gnu/dist
.PATH: ${DIST}/sendmail/doc/intro

DIR=	smm/09.sendmail
SRCS=	intro.me
MACROS=	-me
RM=	rm -f

all: intro.ps

intro.ps: ${SRCS}
	${_MKTARGET_FORMAT}
	${RM} ${.TARGET}
	${TOOL_PIC} ${.ALLSRC} | ${TOOL_ROFF_PS} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
