# $NetBSD: Makefile.inc,v 1.3 2003/10/21 10:01:21 lukem Exp $

.PATH: ${.CURDIR}/KS

SRCS_mapper.dir+=	mapper.dir.KS
SRCS_charset.pivot+=	charset.pivot.KS
CLEANFILES+=		mapper.dir.KS charset.pivot.KS
MAPS_KS=		KSC5601%UCS.mps \
			UCS%KSC5601.mps

mapper.dir.KS: ${.CURDIR}/KS/mapper.dir.KS.src
	${_MKTARGET_CREATE}
	(echo "# KS" ; cat ${.ALLSRC} ; echo ) > ${.TARGET}

charset.pivot.KS: ${.CURDIR}/KS/charset.pivot.KS.src
	${_MKTARGET_CREATE}
	(echo "# KS" ; cat ${.ALLSRC} ; echo ) > ${.TARGET}

FILES+= ${MAPS_KS}
CLEANFILES+= ${MAPS_KS}
.for i in ${MAPS_KS}
FILESDIR_$i= ${BINDIR}/KS
.endfor
