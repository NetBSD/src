#	$NetBSD: Makefile,v 1.2 1998/01/09 08:10:35 perry Exp $
#
#	@(#)Makefile	8.2 (Berkeley) 2/28/94

SRCS=	usenix.me
MACROS=	-me

all: usenix.ps

usenix.ps: ${SRCS}
	rm -f ${.TARGET}
	${PIC} ${SRCS} | ${ROFF} > ${.TARGET}

.include <bsd.doc.mk>
