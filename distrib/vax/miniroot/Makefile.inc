# $NetBSD: Makefile.inc,v 1.15 2002/11/19 16:31:49 bouyer Exp $

IMAGESIZE=	7m
MAKEFS_FLAGS=	-o density=4k
IMAGEENDIAN=	le
MAKEDEVTARGETS=	all
LISTS+=		${DISTRIBDIR}/common/list.sysinst
IMAGEDEPENDS+=	${ARCHDIR}/../install.md \
		${ARCHDIR}/dot.profile \
		${DESTDIR}/etc/disktab \
		${DESTDIR}/usr/mdec/boot \
		${KERNOBJDIR}/GENERIC/netbsd \
		${NETBSDSRCDIR}/share/termcap/termcap.src

IMAGEPOSTBUILD=	${INSTALLBOOT} -v -m ${MACHINE} ${IMAGE} \
		    ${DESTDIR}/usr/mdec/xxboot
