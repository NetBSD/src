#	@(#)bsd.doc.mk	5.3 (Berkeley) 1/2/91

PRINTER=ps

BIB?=		bib
EQN?=		deqn -P${PRINTER}
GREMLIN?=	grn -P${PRINTER}
GRIND?=		vgrind -f
INDXBIB?=	indxbib
PIC?=		pic -P${PRINTER}
REFER?=		refer
ROFF?=		ditroff -t ${MACROS} ${PAGES} -P${PRINTER}
SOELIM?=	soelim
TBL?=		dtbl -P${PRINTER}

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
	rm -rf obj

FILES?=	${SRCS}
install:
	@if [ ! -d "${DESTDIR}${BINDIR}/${DIR}" ]; then \
                /bin/rm -f ${DESTDIR}${BINDIR}/${DIR}  ; \
                mkdir -p ${DESTDIR}${BINDIR}/${DIR}  ; \
                chown root.wheel ${DESTDIR}${BINDIR}/${DIR}  ; \
                chmod 755 ${DESTDIR}${BINDIR}/${DIR}  ; \
        else \
                true ; \
        fi
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m 444 \
	    Makefile ${FILES} ${EXTRA} ${DESTDIR}${BINDIR}/${DIR}

spell: ${SRCS}
	spell ${SRCS} | sort | comm -23 - spell.ok > paper.spell

BINDIR?=	/usr/share/doc
BINGRP?=	bin
BINOWN?=	bin
BINMODE?=	444
