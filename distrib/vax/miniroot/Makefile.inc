# $NetBSD: Makefile.inc,v 1.14 2002/05/29 04:11:03 lukem Exp $

IMAGESIZE=	7m
MAKEFS_FLAGS=	-o density=5k
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
