#	$NetBSD: Makefile,v 1.11 2002/04/26 03:15:15 lukem Exp $

TOP=		${.CURDIR}/..

.include <bsd.own.mk>
.include <bsd.kernobj.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

IMAGE=		ramdisk.fs
IMAGESIZE=	400k
MAKEFS_FLAGS=	-f 15 -o minfree=0,optimization=space,density=4096,cpg=20

WARNS=		1
DBG=		-Os

CRUNCHBIN=	rd_bin
LISTS=		${.CURDIR}/list
MTREECONF=	${.CURDIR}/mtree.conf
IMAGEENDIAN=	be
PARSELISTENV=	TOPDIR=${TOP}
MAKEDEVTARGETS=	__ramdisk std md0 cd0 st0 st1 sd0 sd1 sd2 xd0 xy0
IMAGEDEPENDS=	${CRUNCHBIN} \
		${TOPDIR}/common/rd.disktab ${TOPDIR}/common/rd.fstab \
		${TOPDIR}/common/rd.passwd ${TOPDIR}/common/rd.protocols \
		${TOPDIR}/common/rd.services ${TOPDIR}/common/rd.sshrc \
		${TOPDIR}/common/rd.welcome \
		${_SRC_TOP_}/etc/netconfig

MDSETTARGETS=	RAMDISK ramdisk.fs -
MDSETDIR=	binary/kernel

# Use stubs to eliminate some large stuff from libc
HACKSRC=	${DISTRIBDIR}/utils/libhack
.include	"${HACKSRC}/Makefile.inc"
${CRUNCHBIN}:	libhack.o

.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"
.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
