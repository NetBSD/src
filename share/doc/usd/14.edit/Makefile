#	from: @(#)Makefile	6.2 (Berkeley) 6/30/90
#	$Id: Makefile,v 1.6 1994/01/24 22:37:44 cgd Exp $

DIR=	usd/14.edit
DOC=	14.edit

SRCS=	edittut.ms
MACROS=	-ms
CLEANFILES+=	editvindex

.include <bsd.doc.mk>

${DOC}.${PRINTER_TYPE}: ${SRCS}
	( cd ${.CURDIR} ; ${TBL} ${SRCS} ) | ${ROFF} > ${.TARGET}

# index for versatec is different from the one in edit.tut
# because the fonts are different and entries reference page
# rather than section numbers.  if you have a typesetter
# you should just use the index in edit.tut, and ignore editvindex.

editvindex:
	( cd ${.CURDIR} ; ${ROFF} ${MACROS} -n22 edit.vindex ) > ${.TARGET}
