#	$NetBSD: Makefile,v 1.3.12.1 2002/06/05 05:55:42 lukem Exp $
#
#	@(#)Makefile	8.1 (Berkeley) 8/14/93

DIR=	psd/20.ipctut
SRCS=	tutor.me
MACROS=	-me
EXTRA=	dgramread.c dgramsend.c fig2.pic fig3.pic fig8.pic pipe.c \
	socketpair.c strchkread.c streamread.c streamwrite.c \
	udgramread.c udgramsend.c ustreamread.c ustreamwrite.c

paper.ps: ${SRCS} ${EXTRA}
	${SOELIM} -I${.CURDIR} ${.ALLSRC:M*.me} | ${PIC} | ${TBL} | \
	    ${ROFF} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
