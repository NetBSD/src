#	$NetBSD: bsd.doc.mk,v 1.27 1997/05/06 21:29:33 mycroft Exp $
#	@(#)bsd.doc.mk	8.1 (Berkeley) 8/14/93

.MAIN:		all
.PHONY:		print docinstall spell

.include <bsd.own.mk>

BIB?=		bib
EQN?=		eqn
GREMLIN?=	grn
GRIND?=		vgrind -f
INDXBIB?=	indxbib
PIC?=		pic
REFER?=		refer
ROFF?=		groff -M/usr/share/tmac ${MACROS} ${PAGES}
SOELIM?=	soelim
TBL?=		tbl

BINDIR?=	/usr/share/doc
BINGRP?=	bin
BINOWN?=	bin
BINMODE?=	444

.if !target(all)
all: paper.ps
.endif

.if !target(paper.ps)
paper.ps: ${SRCS}
	${ROFF} ${SRCS} > ${.TARGET}
.endif

.if !target(print)
print: paper.ps
	lpr -P${PRINTER} paper.ps
.endif

.if !target(manpages)
manpages:
.endif

.if !target(obj)
obj:
.endif

clean cleandir:
	rm -f paper.* [eE]rrs mklog ${CLEANFILES}

.if !defined(NODOC)
FILES?=	${SRCS}
.for F in ${FILES} ${EXTRA} Makefile
docinstall:: ${DESTDIR}${BINDIR}/${DIR}/${F}
.if !defined(UPDATE)
.PHONY: ${DESTDIR}${BINDIR}/${DIR}/${F}
.endif
.if !defined(BUILD)
${DESTDIR}${BINDIR}/${DIR}/${F}: .MADE
.endif

.PRECIOUS: ${DESTDIR}${BINDIR}/${DIR}/${F}
${DESTDIR}${BINDIR}/${DIR}/${F}: ${F}
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} ${.ALLSRC} \
		${.TARGET}
.endfor
.else
docinstall::
.endif

install: docinstall

spell: ${SRCS}
	spell ${SRCS} | sort | comm -23 - spell.ok > paper.spell

