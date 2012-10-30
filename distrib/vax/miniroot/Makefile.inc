# $NetBSD: Makefile.inc,v 1.20.6.1 2012/10/30 18:48:56 yamt Exp $

IMAGESIZE=	7m
MAKEFS_FLAGS=	-o density=3k
IMAGEENDIAN=	le
MAKEDEVTARGETS=	all ipty
LISTS+=		${DISTRIBDIR}/common/list.sysinst
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile \
		${DESTDIR}/usr/mdec/boot \
		${KERNOBJDIR}/GENERIC/netbsd

IMAGEPOSTBUILD=	${TOOL_INSTALLBOOT} -v -m ${MACHINE} ${IMAGE} \
		    ${DESTDIR}/usr/mdec/xxboot
