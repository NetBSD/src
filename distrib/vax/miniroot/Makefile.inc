# $NetBSD: Makefile.inc,v 1.24 2013/11/09 21:32:55 christos Exp $

IMAGESIZE=	8m
MAKEFS_FLAGS=	-o density=3k
IMAGEENDIAN=	le
MAKEDEVTARGETS=	all ipty
LISTS+=		${DISTRIBDIR}/common/list.sysinst
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile \
		${DESTDIR}/usr/mdec/boot \
		${KERNOBJDIR}/GENERIC/netbsd

IMAGEPOSTBUILD=	${TOOL_INSTALLBOOT} -v -m ${MACHINE} ${IMAGE} \
		    ${DESTDIR}/usr/mdec/xxboot
