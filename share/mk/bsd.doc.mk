#	$NetBSD: bsd.doc.mk,v 1.53 2001/11/02 05:21:50 tv Exp $
#	@(#)bsd.doc.mk	8.1 (Berkeley) 8/14/93

.include <bsd.init.mk>

##### Basic targets
.PHONY:		cleandoc docinstall print spell
clean:		cleandoc
realinstall:	docinstall

##### Default values
BIB?=		bib
EQN?=		eqn
GREMLIN?=	grn
GRIND?=		vgrind -f
INDXBIB?=	indxbib
PIC?=		pic
REFER?=		refer
ROFF?=		groff -Tps
SOELIM?=	soelim
TBL?=		tbl

##### Build rules
.if !target(paper.ps)
paper.ps: ${SRCS}
	${ROFF} ${MACROS} ${PAGES} ${.ALLSRC} > ${.TARGET}
.endif

.if ${MKSHARE} != "no"
realall:	paper.ps
.endif

##### Install rules
docinstall::	# ensure existence
.if ${MKDOC} != "no"

__docinstall: .USE
	${INSTALL_FILE} -o ${DOCOWN} -g ${DOCGRP} -m ${DOCMODE} \
		${.ALLSRC} ${.TARGET}

FILES?=		${SRCS}

.for F in Makefile ${FILES:O:u} ${EXTRA}
_F:=		${DESTDIR}${DOCDIR}/${DIR}/${F}		# installed path

${_F}:		${F} __docinstall			# install rule
docinstall::	${_F}
.PRECIOUS:	${_F}					# keep if install fails
.PHONY:		${UPDATE:U${_F}}			# clobber unless UPDATE
.if !defined(BUILD) && !make(all) && !make(${F})
${_F}:		.MADE					# no build at install
.endif
.endfor

.undef _F
.endif # ${MKDOC} != "no"

##### Clean rules
cleandoc:
	rm -f paper.* [eE]rrs mklog ${CLEANFILES}

##### Custom rules
.if !target(print)
print: paper.ps
	lpr -P${PRINTER} ${.ALLSRC}
.endif

spell: ${SRCS}
	spell ${.ALLSRC} | sort | comm -23 - spell.ok > paper.spell

##### Pull in related .mk logic
.include <bsd.obj.mk>

${TARGETS}:	# ensure existence
