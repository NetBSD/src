#	$NetBSD: Makefile,v 1.2 1998/01/09 08:10:27 perry Exp $
#
#	@(#)Makefile	8.2 (Berkeley) 2/28/94

DIR=	smm/09.sendmail
SRCS=	intro.me
MACROS=	-me

all: intro.ps

intro.ps: ${SRCS}
	rm -f ${.TARGET}
	${PIC} ${SRCS} | ${ROFF} > ${.TARGET}

.include <bsd.doc.mk>
