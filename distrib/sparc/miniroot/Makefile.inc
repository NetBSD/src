#	$NetBSD: Makefile.inc,v 1.5 2002/04/26 03:49:21 lukem Exp $

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
IMAGE_MD_POST=	${DESTDIR}/usr/mdec/binstall -f miniroot.fs \
		    -m ${DESTDIR}/usr/mdec -i ${SPARCINSTALLBOOT} \
		    -v ffs ${WORKDIR}
