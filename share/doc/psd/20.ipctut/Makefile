#	$NetBSD: Makefile,v 1.5 2003/07/10 10:34:30 lukem Exp $
#
#	@(#)Makefile	8.1 (Berkeley) 8/14/93

DIR=	psd/20.ipctut
SRCS=	tutor.me
MACROS=	-me
EXTRA=	dgramread.c dgramsend.c fig2.pic fig3.pic fig8.pic pipe.c \
	socketpair.c strchkread.c streamread.c streamwrite.c \
	udgramread.c udgramsend.c ustreamread.c ustreamwrite.c

paper.ps: ${SRCS} ${EXTRA}
	${TOOL_SOELIM} -I${.CURDIR} ${.ALLSRC:M*.me} | ${TOOL_PIC} | \
	    ${TOOL_TBL} | ${TOOL_ROFF_PS} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
