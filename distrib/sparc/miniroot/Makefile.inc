#	$NetBSD: Makefile.inc,v 1.1 2002/03/06 12:04:54 lukem Exp $

IMAGESIZE=	7296k
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all
LISTS+=		${DISTRIBDIR}/common/list.sysinst
MAKEFS_FLAGS=   -o density=4k
IMAGEDEPENDS+=	${ARCHDIR}/../install.md ${ARCHDIR}/dot.profile \
		${ARCHDIR}/termcap.src \
		${DESTDIR}/etc/disktab ${DESTDIR}/etc/spwd.db \
		${DESTDIR}/usr/mdec/boot ${DESTDIR}/usr/share/misc/magic \
		${KERNOBJDIR}/GENERIC/netbsd

# XXXIMAGE: binstall needs to be MI and work on image files
image_md_post:
	${DESTDIR}/usr/mdec/binstall -f miniroot.fs -m ${DESTDIR}/usr/mdec \
	    -i ${SPARCINSTALLBOOT} -v ffs ${TARGDIR}
