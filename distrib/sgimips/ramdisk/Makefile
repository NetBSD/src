#	$NetBSD: Makefile,v 1.3 2003/02/01 05:03:43 tsutsui Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

IMAGE=		ramdisk.fs
IMAGETAR=	diskimage.tgz
IMAGESIZE=	3m
MAKEFS_FLAGS=	-f 15 -o density=4k

WARNS=		1
DBG=		-Os

CRUNCHBIN=	ramdiskbin
CRUNCHGEN_FLAGS= -d "${DBG}"
LISTS=		${.CURDIR}/list ${DISTRIBDIR}/common/list.sysinst
MTREECONF=	${DISTRIBDIR}/common/mtree.common
IMAGEENDIAN=	be
MAKEDEVTARGETS=	minimal pty0
IMAGEDEPENDS=	${CRUNCHBIN} \
		disktab.preinstall dot.hdprofile dot.profile \
		${.CURDIR}/../miniroot/termcap.iris-ansi \
		${DESTDIR}/usr/mdec/boot ${DESTDIR}/usr/mdec/boot.elf \
		${NETBSDSRCDIR}/etc/group ${NETBSDSRCDIR}/etc/master.passwd \
		${NETBSDSRCDIR}/etc/netconfig ${NETBSDSRCDIR}/etc/protocols \
		${NETBSDSRCDIR}/etc/services

IMAGETAR_RELEASEDIR=	installation/netboot

# Use stubs to eliminate some large stuff from libc
HACKSRC=	${DISTRIBDIR}/utils/libhack
.include	"${HACKSRC}/Makefile.inc"
${CRUNCHBIN}:	libhack.o

.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

.include <bsd.prog.mk>
