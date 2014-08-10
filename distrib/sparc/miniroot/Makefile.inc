#	$NetBSD: Makefile.inc,v 1.20.2.1 2014/08/10 06:48:41 tls Exp $

IMAGESIZE=	9216k
DBG=		-Os -fno-unwind-tables
MAKEFS_FLAGS=   -o density=4k
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all ipty
LISTS+=		${DISTRIBDIR}/common/list.sysinst
IMAGEDEPENDS+=	${ARCHDIR}/../install.md ${ARCHDIR}/dot.profile \
		${DESTDIR}/usr/mdec/boot \
		${KERNOBJDIR}/GENERIC/netbsd

IMAGEPOSTBUILD=	${TOOL_INSTALLBOOT} -v -m ${MACHINE} ${IMAGE} \
		    ${DESTDIR}/usr/mdec/bootxx /boot
