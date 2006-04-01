# $NetBSD: Makefile.inc,v 1.1 2006/04/01 04:13:22 tnozaki Exp $

.PATH: ${.CURDIR}/AST

CODE:=			ARMSCII
PARTFILE_ARMSCII:=	${.CURDIR}/AST/ARMSCII.part
ALIASFILE_ARMSCII:=	${.CURDIR}/AST/ARMSCII.alias
SUBDIR_ARMSCII:=	AST
.include "${.CURDIR}/Makefile.part"

.for i in ${ARMSCII_PART}
.if !exists(${.CURDIR}/AST/ARMSCII-${i:S/:/@/}.src)
ARMSCII-${i:S/:/@/}.src: ARMSCII.src
	${_MKTARGET_CREATE}
	sed 's/ARMSCII-x/ARMSCII-$i/' $> > $@
CLEANFILES+= ARMSCII-${i:S/:/@/}.src
.endif
.endfor
