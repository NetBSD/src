#	$NetBSD: Makefile,v 1.2 1998/01/09 08:10:23 perry Exp $
#
#	@(#)Makefile	8.1 (Berkeley) 4/13/94

DIR=	smm/09.sendmail
SRCS=	changes.me
MACROS=	-me

all: changes.ps

changes.ps: ${SRCS}
	rm -f ${.TARGET}
	${PIC} ${SRCS} | ${ROFF} > ${.TARGET}

.include <bsd.doc.mk>
