#	from: @(#)bsd.doc.mk	5.3 (Berkeley) 1/2/91
#	$Id: bsd.doc.mk,v 1.16 1994/01/24 22:30:32 cgd Exp $

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
	@install -d -o root -g wheel -m 755 ${DESTDIR}${DOCDIR}/${DIR}
	(cd ${.CURDIR}; install ${COPY} -o ${DOCOWN} -g ${DOCGRP} -m 444 \
	    Makefile ${FILES} ${EXTRA} ${DESTDIR}${DOCDIR}/${DIR} )

spell: ${SRCS}
	(cd ${.CURDIR}; spell ${SRCS}) | sort | \
		comm -23 - ${.CURDIR}/spell.ok > ${DOC}.spell

depend tags lint:

.include <bsd.obj.mk>
