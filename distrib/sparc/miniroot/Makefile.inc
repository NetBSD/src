#	$NetBSD: Makefile.inc,v 1.6 2002/05/02 18:02:54 lukem Exp $

IMAGESIZE=	7296k
MAKEFS_FLAGS=   -o density=4k
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all
LISTS+=		${DISTRIBDIR}/common/list.sysinst
IMAGEDEPENDS+=	${ARCHDIR}/../install.md ${ARCHDIR}/dot.profile \
		${ARCHDIR}/termcap.src \
		${DESTDIR}/etc/disktab ${DESTDIR}/etc/spwd.db \
		${DESTDIR}/usr/mdec/boot ${DESTDIR}/usr/share/misc/magic \
		${KERNOBJDIR}/GENERIC/netbsd

# XXXDISTRIB: binstall needs to be MI and work on image files
IMAGEPOSTBUILD=	${DESTDIR}/usr/mdec/binstall -f ${IMAGE} \
		    -m ${DESTDIR}/usr/mdec -i ${SPARCINSTALLBOOT} \
		    -v ffs ${WORKDIR}
