#	@(#)bsd.doc.mk	5.3 (Berkeley) 1/2/91

PRINTER=ps

# Why d'command'?
#EQN?=		deqn -P${PRINTER}
#TBL?=		dtbl -P${PRINTER}
#ROFF?=		ditroff -t ${MACROS} ${PAGES} -P${PRINTER}

BIB?=		bib
EQN?=		eqn -P${PRINTER}
GREMLIN?=	grn -P${PRINTER}
GRIND?=		vgrind -f
INDXBIB?=	indxbib
PIC?=		pic -P${PRINTER}
REFER?=		refer
ROFF?=		ntroff -t ${MACROS} ${PAGES} -P${PRINTER}
SOELIM?=	soelim
TBL?=		tbl -P${PRINTER}

.PATH: ${.CURDIR}

.if !target(print)
print: paper.${PRINTER}
	lpr -P${PRINTER} paper.${PRINTER}
.endif

.if !target(obj)
obj:
.endif

clean cleandir:
	rm -f paper.* [eE]rrs mklog ${CLEANFILES}

FILES?=	${SRCS}
install:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
	    Makefile ${FILES} ${EXTRA} ${DESTDIR}${BINDIR}/${DIR}

spell: ${SRCS}
	spell ${SRCS} | sort | comm -23 - spell.ok > paper.spell

BINDIR?=	/usr/share/doc
BINGRP?=	bin
BINOWN?=	bin
BINMODE?=	444
