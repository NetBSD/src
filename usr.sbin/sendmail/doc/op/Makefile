#	$NetBSD: Makefile,v 1.2 1998/01/09 08:10:30 perry Exp $
#
#	@(#)Makefile	8.2 (Berkeley) 2/28/94

DIR=	smm/08.sendmailop
SRCS=	op.me
MACROS=	-me

all: op.ps

op.ps: ${SRCS}
	rm -f ${.TARGET}
	${PIC} ${SRCS} | ${EQN} | ${ROFF} > ${.TARGET}

.include <bsd.doc.mk>
