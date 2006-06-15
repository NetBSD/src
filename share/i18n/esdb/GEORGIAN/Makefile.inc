# $NetBSD: Makefile.inc,v 1.1 2006/06/15 15:34:20 tnozaki Exp $

.PATH: ${.CURDIR}/GEORGIAN

CODE:=		GEORGIAN
SEP_GEORGIAN:=	-
.include "${.CURDIR}/Makefile.part"

.for i in ${GEORGIAN_PART}
GEORGIAN-${i}.src: GEORGIAN.src
	${_MKTARGET_CREATE}
	sed -e 's/GEORGIANx/GEORGIAN-${i}/' \
	    -e 's/GEORGIANy/GEORGIAN-${i:S/-/:/}/' $> > $@
CLEANFILES+= GEORGIAN-${i}.src
.endfor
