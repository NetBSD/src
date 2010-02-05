# $NetBSD: Makefile.inc,v 1.19 2010/02/05 10:01:21 roy Exp $

IMAGESIZE=	7m
MAKEFS_FLAGS=	-o density=4k
IMAGEENDIAN=	le
MAKEDEVTARGETS=	all
LISTS+=		${DISTRIBDIR}/common/list.sysinst
IMAGEDEPENDS+=	${ARCHDIR}/../install.md \
		${ARCHDIR}/dot.profile \
		${DESTDIR}/usr/mdec/boot \
		${KERNOBJDIR}/GENERIC/netbsd \
		${DESTDIR}/rescue/terminfo.db

IMAGEPOSTBUILD=	${TOOL_INSTALLBOOT} -v -m ${MACHINE} ${IMAGE} \
		    ${DESTDIR}/usr/mdec/xxboot
