#	@(#)bsd.doc.mk	5.3 (Berkeley) 1/2/91

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

BINDIR?=	/usr/share/doc
BINGRP?=	bin
BINOWN?=	bin
BINMODE?=	444

.PATH: ${.CURDIR}

all:	${DOC}.${PRINTER}

.if !target(print)
print: ${DOC}.${PRINTER}
	lpr -P${PRINTER} ${DOC}.${PRINTER}
.endif

clean cleandir:
	rm -f ${DOC}.* [eE]rrs mklog ${CLEANFILES}

FILES?=	${SRCS}
install:
	@install -d -o root -g wheel -m 755 ${DESTDIR}${BINDIR}/${DIR}
	(cd ${.CURDIR}; install ${COPY} -o ${BINOWN} -g ${BINGRP} -m 444 \
	    Makefile ${FILES} ${EXTRA} ${DESTDIR}${BINDIR}/${DIR} )

spell: ${SRCS}
	(cd ${.CURDIR}; spell ${SRCS}) | sort | \
		comm -23 - ${.CURDIR}/spell.ok > ${DOC}.spell

.include <bsd.obj.mk>
