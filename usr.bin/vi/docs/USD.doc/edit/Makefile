#	from: @(#)Makefile	8.1 (Berkeley) 6/8/93
#	$Id: Makefile,v 1.2 1994/01/24 06:39:37 cgd Exp $

DIR=	usd/11.edit
SRCS=	edittut.ms
MACROS=	-msU

paper.ps: ${SRCS}
	${TBL} ${SRCS} | ${ROFF} > ${.TARGET}

# index for versatec is different from the one in edit.tut
# because the fonts are different and entries reference page
# rather than section numbers.  if you have a typesetter
# you should just use the index in edit.tut, and ignore editvindex.

editvindex:
	${TROFF} ${MACROS} -n22 edit.vindex

.include <bsd.doc.mk>
