#	$NetBSD: Makefile.inc,v 1.9.2.1 2002/05/29 04:59:31 lukem Exp $

IMAGESIZE=	7680k
MAKEFS_FLAGS=   -o density=4k
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all
LISTS+=		${DISTRIBDIR}/common/list.sysinst
IMAGEDEPENDS+=	${ARCHDIR}/../install.md ${ARCHDIR}/dot.profile \
		${ARCHDIR}/termcap.src \
		${DESTDIR}/etc/disktab \
		${DESTDIR}/usr/mdec/boot ${DESTDIR}/usr/share/misc/magic \
		${KERNOBJDIR}/GENERIC/netbsd

IMAGEPOSTBUILD=	${INSTALLBOOT} -v -m ${MACHINE} ${IMAGE} \
		    ${DESTDIR}/usr/mdec/bootxx /boot
