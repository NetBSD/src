# $NetBSD: Makefile.inc,v 1.1 2003/06/27 08:40:31 tshiozak Exp $

.PATH: ${.CURDIR}/GB

SRCS_mapper.dir+=	mapper.dir.GB
SRCS_charset.pivot+=	charset.pivot.GB
CLEANFILES+=		mapper.dir.GB charset.pivot.GB
MAPS_GB=		GB2312%UCS.mps \
			UCS%GB2312.mps

mapper.dir.GB: ${.CURDIR}/GB/mapper.dir.GB.src
	echo "# GB" > $@
	cat $> >> $@
	echo >> $@

charset.pivot.GB: ${.CURDIR}/GB/charset.pivot.GB.src
	echo "# GB" > $@
	cat $> >> $@
	echo >> $@

FILES+= ${MAPS_GB}
CLEANFILES+= ${MAPS_GB}
.for i in ${MAPS_GB}
FILESDIR_$i= ${BINDIR}/GB
.endfor
