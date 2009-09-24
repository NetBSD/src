#	$NetBSD: Makefile,v 1.29 2009/09/24 21:04:46 he Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

IMAGE=		ramdisk.fs
IMAGESIZE=	2560k
MAKEFS_FLAGS=	-f 15

WARNS=		1
DBG=		-Os -mmemcpy

CRUNCHBIN=	ramdiskbin
CRUNCHGEN_FLAGS= -d "${DBG}"
LISTS=		${.CURDIR}/list ${DISTRIBDIR}/common/list.sysinst
MTREECONF=	${DISTRIBDIR}/common/mtree.common
IMAGEENDIAN=	be
MAKEDEVTARGETS=	floppy
IMAGEDEPENDS=	${CRUNCHBIN} \
		dot.profile ${DISTRIBDIR}/common/termcap \
		${DESTDIR}/usr/mdec/boot ${DESTDIR}/usr/mdec/bootxx \
		${NETBSDSRCDIR}/etc/group ${NETBSDSRCDIR}/etc/master.passwd \
		${NETBSDSRCDIR}/etc/netconfig ${DISTRIBDIR}/common/protocols \
		${DISTRIBDIR}/common/services

# Use stubs to eliminate some large stuff from libc
HACKSRC=	${DISTRIBDIR}/utils/libhack
.include	"${HACKSRC}/Makefile.inc"
${CRUNCHBIN}:	libhack.o

.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

release:

.include <bsd.prog.mk>
