# $NetBSD: Makefile.inc,v 1.1 2003/06/27 08:40:53 tshiozak Exp $

.PATH: ${.CURDIR}/EBCDIC

CODE:=		EBCDIC
.include "${.CURDIR}/Makefile.part"

.for i in ${EBCDIC_PART}
EBCDIC-${i:S/:/@/}.src: EBCDIC.src
	sed 's/EBCDIC-x/EBCDIC-$i/' $> > $@
CLEANFILES:= ${CLEANFILES} EBCDIC-${i:S/:/@/}.src
.endfor
