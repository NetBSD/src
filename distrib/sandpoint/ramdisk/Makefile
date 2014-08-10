#	$NetBSD: Makefile,v 1.10.10.1 2014/08/10 06:48:29 tls Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

IMAGE=		ramdisk.fs
IMAGESIZE=	2048k
MAKEFS_FLAGS=	-f 15

WARNS=		1
DBG=		-Os

CRUNCHBIN=	ramdiskbin
CRUNCHGEN_FLAGS=-d "${DBG}"
LISTS=		${.CURDIR}/list ${DISTRIBDIR}/common/list.sysinst
MTREECONF=	${DISTRIBDIR}/common/mtree.common
IMAGEENDIAN=	be
MAKEDEVTARGETS=	std md0 wd0 wd1 wd2 wd3 sd0 cd0 ipty satmgr raid0 raid1
IMAGEDEPENDS=	${CRUNCHBIN} \
		dot.profile \
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
