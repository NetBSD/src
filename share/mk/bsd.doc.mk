#	$NetBSD: bsd.doc.mk,v 1.41 1999/02/12 01:10:06 lukem Exp $
#	@(#)bsd.doc.mk	8.1 (Berkeley) 8/14/93

.if !target(__initialized__)
__initialized__:
.if exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif
.include <bsd.own.mk>
.MAIN:		all
.endif

.PHONY:		cleandoc docinstall print spell
.if ${MKSHARE} != "no"
realinstall:	docinstall
.endif
clean cleandir distclean: cleandoc

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

.if !target(all)
.if ${MKSHARE} != "no"
all: paper.ps
.else
all:
.endif
.endif

.if !target(paper.ps)
paper.ps: ${SRCS}
	${ROFF} ${.ALLSRC} > ${.TARGET}
.endif

.if !target(print)
print: paper.ps
	lpr -P${PRINTER} ${.ALLSRC}
.endif

cleandoc:
	rm -f paper.* [eE]rrs mklog ${CLEANFILES}

.if ${MKDOC} != "no"
FILES?=	${SRCS}
.for F in ${FILES} ${EXTRA} Makefile
docinstall:: ${DESTDIR}${DOCDIR}/${DIR}/${F}
.if !defined(UPDATE)
.PHONY: ${DESTDIR}${DOCDIR}/${DIR}/${F}
.endif
.if !defined(BUILD)
${DESTDIR}${DOCDIR}/${DIR}/${F}: .MADE
.endif

.PRECIOUS: ${DESTDIR}${DOCDIR}/${DIR}/${F}
${DESTDIR}${DOCDIR}/${DIR}/${F}: ${F}
	${INSTALL} ${RENAME} ${PRESERVE} -c -o ${DOCOWN} -g ${DOCGRP} \
		-m ${DOCMODE} ${.ALLSRC} ${.TARGET}
.endfor
.endif

.if !target(docinstall)
docinstall::
.endif

spell: ${SRCS}
	spell ${.ALLSRC} | sort | comm -23 - spell.ok > paper.spell

depend includes lint obj tags:

.include <bsd.obj.mk>
