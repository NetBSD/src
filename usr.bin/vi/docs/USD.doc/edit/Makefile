#	$NetBSD: Makefile,v 1.8 2003/07/10 10:34:45 lukem Exp $
#
#	@(#)Makefile	8.1 (Berkeley) 6/8/93

DIR=	usd/11.edit
SRCS=	edittut.ms
MACROS=	-ms

all: paper.ps

paper.ps: ${SRCS}
	${TOOL_TBL} ${SRCS} | ${TOOL_ROFF_PS} ${MACROS} > ${.TARGET}

# index for versatec is different from the one in edit.tut
# because the fonts are different and entries reference page
# rather than section numbers.  if you have a typesetter
# you should just use the index in edit.tut, and ignore editvindex.

editvindex:
	${TOOL_ROFF_RAW} ${MACROS} -n22 edit.vindex

.include <bsd.doc.mk>
