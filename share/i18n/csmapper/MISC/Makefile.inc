# $NetBSD: Makefile.inc,v 1.1 2003/06/27 08:40:49 tshiozak Exp $

.PATH: ${.CURDIR}/MISC

SRCS_mapper.dir+=	mapper.dir.MISC
SRCS_charset.pivot+=	charset.pivot.MISC
CLEANFILES+=		mapper.dir.MISC charset.pivot.MISC
MAPS_MISC=		Big5%UCS.mps \
			UCS%Big5.mps

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
