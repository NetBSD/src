# $NetBSD: Makefile.inc,v 1.6 2006/03/28 14:44:01 tnozaki Exp $

.PATH: ${.CURDIR}/MISC

SRCS_mapper.dir+=	mapper.dir.MISC
SRCS_charset.pivot+=	charset.pivot.MISC
CLEANFILES+=		mapper.dir.MISC charset.pivot.MISC
MAPS_MISC=		Big5%UCS.mps \
			UCS%Big5.mps \
			KOI8-R%UCS.mps \
			UCS%KOI8-R.mps \
			KOI8-U%UCS.mps \
			UCS%KOI8-U.mps \
			PTCP154%UCS.mps \
			UCS%PTCP154.mps

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
