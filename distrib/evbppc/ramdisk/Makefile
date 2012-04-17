#	$NetBSD: Makefile,v 1.10.2.1 2012/04/17 00:02:33 yamt Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

IMAGE=		ramdisk.fs
IMAGESIZE=	2304k
MAKEFS_FLAGS=	-f 15

WARNS=		1
DBG=		-Os -mmultiple -mno-string -fno-pie -fno-pic

CRUNCHBIN=	ramdiskbin
CRUNCHGEN_FLAGS=-d "${DBG}"
LISTS=		${.CURDIR}/list ${DISTRIBDIR}/common/list.sysinst
MTREECONF=	${DISTRIBDIR}/common/mtree.common
IMAGEENDIAN=	be
IMAGEDEPENDS=	${CRUNCHBIN} \
		dot.profile \
		${NETBSDSRCDIR}/etc/group ${NETBSDSRCDIR}/etc/master.passwd \
		${NETBSDSRCDIR}/etc/netconfig ${DISTRIBDIR}/common/protocols \
		${DISTRIBDIR}/common/services
MAKEDEVTARGETS=	std md0 wd0 wd1 wd2 wd3 cd0 cd1 sd0 sd1 sd2 sd3 st0 pty0
MAKEDEVTARGETS+= ld0 ld1

# Use stubs to eliminate some large stuff from libc
HACKSRC=	${DISTRIBDIR}/utils/libhack
.include	"${HACKSRC}/Makefile.inc"
${CRUNCHBIN}:	libhack.o

.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

release:

.include <bsd.prog.mk>
