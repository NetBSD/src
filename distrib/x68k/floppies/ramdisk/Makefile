#	$NetBSD: Makefile,v 1.21 2002/05/13 01:04:23 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

IMAGE=			inst.fs
IMAGESIZE=		1200k
IMAGE_RELEASEDIR=	installation/floppy

MAKEFS_FLAGS=	-f 10
WARNS=		1
DBG=		-Os

CRUNCHBIN=	ramdiskbin
LISTS=		${.CURDIR}/list ${DISTRIBDIR}/common/list.sysinst
MTREECONF=	${DISTRIBDIR}/common/mtree.common
IMAGEENDIAN=	be
MAKEDEVTARGETS=	std md0 fd0 sd0 sd1 sd2 sd3 sd4 sd5 cd0 ttye0 tty00 opty
IMAGEDEPENDS+= 	${CRUNCHBIN} \
		disktab.preinstall dot.profile dot.hdprofile \
		services termcap.vt \
		${DESTDIR}/usr/mdec/boot ${DESTDIR}/usr/mdec/mboot \
		${DESTDIR}/usr/mdec/sdboot_ufs \
		${NETBSDSRCDIR}/etc/group ${NETBSDSRCDIR}/etc/master.passwd \
		${NETBSDSRCDIR}/etc/netconfig ${NETBSDSRCDIR}/etc/protocols

# Use stubs to eliminate some large stuff from libc
HACKSRC=	${DISTRIBDIR}/utils/libhack
.include	"${HACKSRC}/Makefile.inc"
${CRUNCHBIN}:	libhack.o

.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

release::

.include <bsd.prog.mk>
