#	$NetBSD: Makefile,v 1.7 2003/07/10 10:34:21 lukem Exp $

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
	${TOOL_PIC} ${.ALLSRC} | ${TOOL_EQN} | \
	    ${TOOL_ROFF_PS} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
