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

.PATH: ${.CURDIR}

all:	${DOC}.${PRINTER}

.if !target(print)
print: ${DOC}.${PRINTER}
	lpr -P${PRINTER} ${DOC}.${PRINTER}
.endif

.if !target(obj)
obj:
.endif

clean cleandir:
	rm -f ${DOC}.* [eE]rrs mklog ${CLEANFILES}
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
	spell ${SRCS} | sort | comm -23 - spell.ok > ${DOC}.spell

BINDIR?=	/usr/share/doc
BINGRP?=	bin
BINOWN?=	bin
BINMODE?=	444
