# $NetBSD: Makefile.inc,v 1.2 2003/10/19 14:08:50 lukem Exp $

.PATH: ${.CURDIR}/GB

SRCS_mapper.dir+=	mapper.dir.GB
SRCS_charset.pivot+=	charset.pivot.GB
CLEANFILES+=		mapper.dir.GB charset.pivot.GB
MAPS_GB=		GB2312%UCS.mps \
			UCS%GB2312.mps

mapper.dir.GB: ${.CURDIR}/GB/mapper.dir.GB.src
	${_MKMSGCREATE}
	${_MKCMD}\
	(echo "# GB" ; cat ${.ALLSRC} ; echo ) > ${.TARGET}

charset.pivot.GB: ${.CURDIR}/GB/charset.pivot.GB.src
	${_MKMSGCREATE}
	${_MKCMD}\
	(echo "# GB" ; cat ${.ALLSRC} ; echo ) > ${.TARGET}

FILES+= ${MAPS_GB}
CLEANFILES+= ${MAPS_GB}
.for i in ${MAPS_GB}
FILESDIR_$i= ${BINDIR}/GB
.endfor
