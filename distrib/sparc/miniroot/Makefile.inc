#	$NetBSD: Makefile.inc,v 1.12 2004/06/13 05:27:18 sjg Exp $

IMAGESIZE=	8192k
MAKEFS_FLAGS=   -o density=4k
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all
LISTS+=		${DISTRIBDIR}/common/list.sysinst
IMAGEDEPENDS+=	${ARCHDIR}/../install.md ${ARCHDIR}/dot.profile \
		${ARCHDIR}/termcap.src \
		${DESTDIR}/etc/disktab \
		${DESTDIR}/usr/mdec/boot ${DESTDIR}/usr/share/misc/magic \
		${KERNOBJDIR}/GENERIC/netbsd

IMAGEPOSTBUILD=	${TOOL_INSTALLBOOT} -v -m ${MACHINE} ${IMAGE} \
		    ${DESTDIR}/usr/mdec/bootxx /boot
