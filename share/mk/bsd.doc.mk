#	$NetBSD: bsd.doc.mk,v 1.49 2001/05/08 03:19:51 sommerfeld Exp $
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
realall: paper.ps
.else
realall:
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
FILES?=${SRCS}
ALLFILES=Makefile ${FILES} ${EXTRA}

docinstall:: ${ALLFILES:@F@${DESTDIR}${DOCDIR}/${DIR}/${F}@}
.PRECIOUS: ${ALLFILES:@F@${DESTDIR}${DOCDIR}/${DIR}/${F}@}
.if !defined(UPDATE)
.PHONY: ${ALLFILES:@F@${DESTDIR}${DOCDIR}/${DIR}/${F}@}
.endif

__docinstall: .USE
	${INSTALL} ${RENAME} ${PRESERVE} ${INSTPRIV} -c -o ${DOCOWN} \
	    -g ${DOCGRP} -m ${DOCMODE} ${.ALLSRC} ${.TARGET}

.for F in ${ALLFILES:O:u}
.if !defined(BUILD) && !make(all) && !make(${F})
${DESTDIR}${DOCDIR}/${DIR}/${F}: .MADE
.endif
${DESTDIR}${DOCDIR}/${DIR}/${F}: ${F} __docinstall
.endfor
.endif

.if !target(docinstall)
docinstall::
.endif

spell: ${SRCS}
	spell ${.ALLSRC} | sort | comm -23 - spell.ok > paper.spell

depend includes lint obj tags:

dependall: all

.include <bsd.obj.mk>
