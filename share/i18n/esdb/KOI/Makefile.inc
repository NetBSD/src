# $NetBSD: Makefile.inc,v 1.1 2006/04/08 14:15:48 tnozaki Exp $

.PATH: ${.CURDIR}/KOI

CODE:=		KOI
SEP_KOI:=
.include "${.CURDIR}/Makefile.part"

.for i in ${KOI_PART}
.if !exists(${.CURDIR}/KOI/KOI${i:S/:/@/}.src)
KOI${i:S/:/@/}.src: KOI.src
	${_MKTARGET_CREATE}
	sed 's/KOIx/KOI$i/' $> > $@
CLEANFILES+= KOI${i:S/:/@/}.src
.endif
.endfor
