# $NetBSD: Makefile.inc,v 1.1 2003/06/27 08:40:57 tshiozak Exp $

.PATH: ${.CURDIR}/ISO646

CODE:=		ISO646
.include "${.CURDIR}/Makefile.part"

.for i in ${ISO646_PART}
ISO646-${i:S/:/@/}.src: ISO646.src
	sed 's/ISO646-x/ISO646-$i/' $> > $@
CLEANFILES:= ${CLEANFILES} ISO646-${i:S/:/@/}.src
.endfor
