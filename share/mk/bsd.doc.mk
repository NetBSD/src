#	from: @(#)bsd.doc.mk	5.3 (Berkeley) 1/2/91
#	$Id: bsd.doc.mk,v 1.17 1994/02/09 23:50:35 cgd Exp $

PRINTER_TYPE?=	ps

BIB?=		bib
EQN?=		eqn
GREMLIN?=	grn
GRIND?=		vgrind -f
INDXBIB?=	indxbib
PIC?=		pic
REFER?=		refer
ROFF?=		groff -T${PRINTER_TYPE} ${MACROS} ${PAGES}
SOELIM?=	soelim
TBL?=		tbl

.PATH: ${.CURDIR}

all:	${DOC}.${PRINTER_TYPE}

.if !target(print)
print: ${DOC}.${PRINTER_TYPE}
	lpr ${DOC}.${PRINTER_TYPE}
.endif

clean:
	rm -f ${DOC}.* [eE]rrs mklog ${CLEANFILES}

cleandir: clean

FILES?=	${SRCS}
install:
	(cd ${.CURDIR}; install ${COPY} -o ${DOCOWN} -g ${DOCGRP} -m 444 \
	    Makefile ${FILES} ${EXTRA} ${DESTDIR}${DOCDIR}/${DIR} )

spell: ${SRCS}
	(cd ${.CURDIR}; spell ${SRCS}) | sort | \
		comm -23 - ${.CURDIR}/spell.ok > ${DOC}.spell

depend tags lint:

.include <bsd.obj.mk>
