# $NetBSD: Makefile.inc,v 1.1 2005/03/27 22:30:06 tnozaki Exp $

.PATH: ${.CURDIR}/CNS

SRCS_mapper.dir+=	mapper.dir.CNS
SRCS_charset.pivot+=	charset.pivot.CNS
CLEANFILES+=		mapper.dir.CNS charset.pivot.CNS
MAPS_CNS=		CNS11643-1%UCS.mps \
			CNS11643-2%UCS.mps \
			UCS%CNS11643-1.mps \
			UCS%CNS11643-2.mps

mapper.dir.CNS: ${.CURDIR}/CNS/mapper.dir.CNS.src
	${_MKTARGET_CREATE}
	(echo "# CNS" ; cat ${.ALLSRC} ; echo ) > ${.TARGET}

charset.pivot.CNS: ${.CURDIR}/CNS/charset.pivot.CNS.src
	${_MKTARGET_CREATE}
	(echo "# CNS" ; cat ${.ALLSRC} ; echo ) > ${.TARGET}

FILES+= ${MAPS_CNS}
CLEANFILES+= ${MAPS_CNS}
.for i in ${MAPS_CNS}
FILESDIR_$i= ${BINDIR}/CNS
.endfor
