# $NetBSD: Makefile.inc,v 1.11 2006/11/29 15:45:26 tnozaki Exp $

.PATH: ${.CURDIR}/MISC

SRCS_mapper.dir+=	mapper.dir.MISC
SRCS_charset.pivot+=	charset.pivot.MISC
CLEANFILES+=		mapper.dir.MISC charset.pivot.MISC
MAPS_MISC=		ATARIST%UCS.mps \
			HP-ROMAN8%UCS.mps \
			MULELAO-1%UCS.mps \
			NEXTSTEP%UCS.mps \
			PTCP154%UCS.mps \
			TDS565%UCS.mps \
			UCS%ATARIST.mps \
			UCS%HP-ROMAN8.mps \
			UCS%MULELAO-1.mps \
			UCS%NEXTSTEP.mps \
			UCS%PTCP154.mps \
			UCS%TDS565.mps

mapper.dir.MISC: ${.CURDIR}/MISC/mapper.dir.MISC.src
	${_MKTARGET_CREATE}
	(echo "# MISC" ; cat ${.ALLSRC} ; echo ) > ${.TARGET}

charset.pivot.MISC: ${.CURDIR}/MISC/charset.pivot.MISC.src
	${_MKTARGET_CREATE}
	(echo "# MISC" ; cat ${.ALLSRC} ; echo ) > ${.TARGET}

FILES+= ${MAPS_MISC}
CLEANFILES+= ${MAPS_MISC}
.for i in ${MAPS_MISC}
FILESDIR_$i= ${BINDIR}/MISC
.endfor
