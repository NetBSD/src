# $NetBSD: Makefile.inc,v 1.3 2003/07/20 19:26:56 tshiozak Exp $

.PATH: ${.CURDIR}/MISC

SRCS_mapper.dir+=	mapper.dir.MISC
SRCS_charset.pivot+=	charset.pivot.MISC
CLEANFILES+=		mapper.dir.MISC charset.pivot.MISC
MAPS_MISC=		Big5%UCS.mps \
			UCS%Big5.mps \
			KOI8-R%UCS.mps \
			UCS%KOI8-R.mps \
			KOI8-U%UCS.mps \
			UCS%KOI8-U.mps

mapper.dir.MISC: ${.CURDIR}/MISC/mapper.dir.MISC.src
	echo "# MISC" > $@
	cat $> >> $@
	echo >> $@

charset.pivot.MISC: ${.CURDIR}/MISC/charset.pivot.MISC.src
	echo "# MISC" > $@
	cat $> >> $@
	echo >> $@

FILES+= ${MAPS_MISC}
CLEANFILES+= ${MAPS_MISC}
.for i in ${MAPS_MISC}
FILESDIR_$i= ${BINDIR}/MISC
.endfor
