# $NetBSD: Makefile.inc,v 1.1 2003/06/27 08:40:48 tshiozak Exp $

.PATH: ${.CURDIR}/KS

SRCS_mapper.dir+=	mapper.dir.KS
SRCS_charset.pivot+=	charset.pivot.KS
CLEANFILES+=		mapper.dir.KS charset.pivot.KS
MAPS_KS=		KSC5601%UCS.mps \
			UCS%KSC5601.mps

mapper.dir.KS: ${.CURDIR}/KS/mapper.dir.KS.src
	echo "# KS" > $@
	cat $> >> $@
	echo >> $@

charset.pivot.KS: ${.CURDIR}/KS/charset.pivot.KS.src
	echo "# KS" > $@
	cat $> >> $@
	echo >> $@

FILES+= ${MAPS_KS}
CLEANFILES+= ${MAPS_KS}
.for i in ${MAPS_KS}
FILESDIR_$i= ${BINDIR}/KS
.endfor
