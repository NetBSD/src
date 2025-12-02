#	$NetBSD: Makefile,v 1.25 2025/12/02 13:28:55 brad Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

IMAGE=		ramdisk.fs
IMAGETAR=	diskimage.tgz
IMAGESIZE=	3500k
MAKEFS_FLAGS+=	-f 15 -o density=4k

WARNS=		1
DBG=		-Os -mmemcpy

CRUNCHBIN=	ramdiskbin
LISTS=		${.CURDIR}/list ${DISTRIBDIR}/common/list.sysinst
MTREECONF=	${DISTRIBDIR}/common/mtree.common
IMAGEENDIAN=	be
IMAGEDEPENDS=	${CRUNCHBIN} \
		dot.profile \
		${DESTDIR}/usr/mdec/aoutboot ${DESTDIR}/usr/mdec/ip2xboot \
		${DESTDIR}/usr/mdec/ip3xboot \
		${NETBSDSRCDIR}/etc/group ${NETBSDSRCDIR}/etc/master.passwd \
		${NETBSDSRCDIR}/etc/netconfig ${DISTRIBDIR}/common/protocols \
		${DISTRIBDIR}/common/services

IMAGETAR_RELEASEDIR=	installation/netboot

# Use stubs to eliminate some large stuff from libc
HACKSRC=	${DISTRIBDIR}/utils/libhack
.include	"${HACKSRC}/Makefile.inc"
${CRUNCHBIN}:	libhack.o

.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.dhcpcd"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

.include <bsd.prog.mk>
