#	$NetBSD: Makefile,v 1.7 2002/08/13 20:10:54 pooka Exp $
#
#	@(#)Makefile	8.1 (Berkeley) 6/8/93

DIR=	usd/11.edit
SRCS=	edittut.ms
MACROS=	-ms

all: paper.ps

paper.ps: ${SRCS}
	${TBL} ${SRCS} | ${ROFF} ${MACROS} > ${.TARGET}

# index for versatec is different from the one in edit.tut
# because the fonts are different and entries reference page
# rather than section numbers.  if you have a typesetter
# you should just use the index in edit.tut, and ignore editvindex.

editvindex:
	${TROFF} ${MACROS} -n22 edit.vindex

.include <bsd.doc.mk>
