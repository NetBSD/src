#	from: @(#)bsd.doc.mk	5.3 (Berkeley) 1/2/91
#	$Id: bsd.doc.mk,v 1.14 1993/08/15 20:42:40 mycroft Exp $

PRINTER?=	ps

BIB?=		bib
EQN?=		eqn
GREMLIN?=	grn
GRIND?=		vgrind -f
INDXBIB?=	indxbib
PIC?=		pic
REFER?=		refer
ROFF?=		groff -T${PRINTER} ${MACROS} ${PAGES}
SOELIM?=	soelim
TBL?=		tbl

.PATH: ${.CURDIR}

all:	${DOC}.${PRINTER}

.if !target(print)
print: ${DOC}.${PRINTER}
	lpr -P${PRINTER} ${DOC}.${PRINTER}
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

.include <bsd.obj.mk>
