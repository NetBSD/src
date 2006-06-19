# $NetBSD: Makefile.inc,v 1.1 2006/06/19 17:28:25 tnozaki Exp $

.PATH: ${.CURDIR}/BIG5

SRCS_mapper.dir+=	mapper.dir.BIG5
SRCS_charset.pivot+=	charset.pivot.BIG5
CLEANFILES+=		mapper.dir.BIG5 charset.pivot.BIG5
MAPS_BIG5=		Big5@1984%UCS.mps \
			Big5EXT@2003%UCS.mps \
			Big5EXT@E%UCS.mps \
			Big5EXT@ETen%UCS.mps \
			Big5EXT@IBM%UCS.mps \
			Big5EXT@Plus%UCS.mps \
			Big5UDA%UCS.mps \
			UCS%Big5@1984.mps \
			UCS%Big5EXT@2003.mps \
			UCS%Big5EXT@E.mps \
			UCS%Big5EXT@ETen.mps \
			UCS%Big5EXT@IBM.mps \
			UCS%Big5EXT@Plus.mps \
			UCS%Big5UDA.mps

mapper.dir.BIG5: ${.CURDIR}/BIG5/mapper.dir.BIG5.src
	${_MKTARGET_CREATE}
	(echo "# BIG5" ; cat ${.ALLSRC} ; echo ) > ${.TARGET}

charset.pivot.BIG5: ${.CURDIR}/BIG5/charset.pivot.BIG5.src
	${_MKTARGET_CREATE}
	(echo "# BIG5" ; cat ${.ALLSRC} ; echo ) > ${.TARGET}

FILES+= ${MAPS_BIG5}
CLEANFILES+= ${MAPS_BIG5}
.for i in ${MAPS_BIG5}
FILESDIR_$i= ${BINDIR}/BIG5
.endfor
