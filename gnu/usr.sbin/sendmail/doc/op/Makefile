#	$NetBSD: Makefile,v 1.8 2003/10/22 06:04:03 lukem Exp $

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
	${_MKTARGET_FORMAT}
	${RM} ${.TARGET}
	${TOOL_PIC} ${.ALLSRC} | ${TOOL_EQN} | \
	    ${TOOL_ROFF_PS} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
