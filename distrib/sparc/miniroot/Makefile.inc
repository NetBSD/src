#	$NetBSD: Makefile.inc,v 1.28.2.1 2018/03/30 06:20:06 pgoyette Exp $

IMAGESIZE=	9800k
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
