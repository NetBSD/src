#	$NetBSD: Makefile,v 1.20 2025/04/22 17:39:12 christos Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

IMAGE=		ramdisk.fs
IMAGESIZE=	4000k
MAKEFS_FLAGS+=	-f 15

WARNS=		1
DBG=		-Os -mmemcpy -fno-unwind-tables

CRUNCHBIN=	ramdiskbin
# Drop IEEE 802.11 support from x_ifconfig.
CRUNCHENV+=	NOIEEE80211=1
LISTS=		${.CURDIR}/list ${DISTRIBDIR}/common/list.sysinst.en
MTREECONF=	${DISTRIBDIR}/common/mtree.common
IMAGEENDIAN=	be
MAKEDEVTARGETS=	ramdisk
IMAGEDEPENDS=	${CRUNCHBIN} \
		dot.profile ${DESTDIR} \
		${DESTDIR}/usr/mdec/boot \
		${DESTDIR}/usr/mdec/bootxx_ustarfs \
		${NETBSDSRCDIR}/etc/group ${NETBSDSRCDIR}/etc/master.passwd \
		${NETBSDSRCDIR}/etc/netconfig ${DISTRIBDIR}/common/protocols \
		${DISTRIBDIR}/common/services

# Use stubs to eliminate some large stuff from libc
HACKSRC=	${DISTRIBDIR}/utils/libhack
HACK_CURSES=	yes
.include	"${HACKSRC}/Makefile.inc"
${CRUNCHBIN}:	libhack.o

.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

release:

.include <bsd.prog.mk>
