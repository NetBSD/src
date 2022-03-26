#	$NetBSD: Makefile.inc,v 1.32 2022/03/26 17:11:20 christos Exp $

IMAGESIZE=	11m
DBG=		${${ACTIVE_CC} == "clang":? -Oz -fomit-frame-pointer : -Os } -fno-unwind-tables

MAKEFS_FLAGS+=   -o density=4k
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all ipty
LISTS+=		${DISTRIBDIR}/common/list.sysinst
IMAGEDEPENDS+=	${ARCHDIR}/../install.md ${ARCHDIR}/dot.profile \
		${DESTDIR}/usr/mdec/boot \
		${KERNOBJDIR}/GENERIC/netbsd

IMAGEPOSTBUILD=	${TOOL_INSTALLBOOT} -v -m ${MACHINE} ${IMAGE} \
		    ${DESTDIR}/usr/mdec/bootxx /boot
