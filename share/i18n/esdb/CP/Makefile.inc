# $NetBSD: Makefile.inc,v 1.1 2003/06/27 08:40:52 tshiozak Exp $

.PATH: ${.CURDIR}/CP

CODE:=		CP
SEP_CP:=
.include "${.CURDIR}/Makefile.part"

.for i in ${CP_PART}
.if !exists(${.CURDIR}/CP/CP${i:S/:/@/}.src)
CP${i:S/:/@/}.src: CP.src
	sed 's/CPx/CP$i/' $> > $@
CLEANFILES+= CP${i:S/:/@/}.src
.endif
.endfor
