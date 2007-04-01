# $NetBSD: Makefile.inc,v 1.1 2007/04/01 18:52:31 tnozaki Exp $

.PATH: ${.CURDIR}/DEC

CODE:=		DEC
SEP_DEC:=
.include "${.CURDIR}/Makefile.part"

.for i in ${DEC_PART}
.if !exists(${.CURDIR}/DEC/DEC${i:S/:/@/}.src)
DEC${i:S/:/@/}.src: DEC.src
	${_MKTARGET_CREATE}
	sed 's/DECx/DEC$i/' $> > $@
CLEANFILES+=	DEC${i:S/:/@/}.src
.endif
.endfor
