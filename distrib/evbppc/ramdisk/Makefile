#	$NetBSD: Makefile,v 1.16.10.1 2014/08/10 06:48:02 tls Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

IMAGE=		ramdisk.fs
IMAGESIZE=	2880k
MAKEFS_FLAGS=	-f 15

WARNS=		1
DBG=		-Os

CRUNCHBIN=	ramdiskbin
CRUNCHGEN_FLAGS=-d "${DBG}"
LISTS=		${.CURDIR}/list ${DISTRIBDIR}/common/list.sysinst
MTREECONF=	${DISTRIBDIR}/common/mtree.common
IMAGEENDIAN=	be
IMAGEDEPENDS=	${CRUNCHBIN} \
		dot.profile \
		Makefile \
		${NETBSDSRCDIR}/etc/group ${NETBSDSRCDIR}/etc/master.passwd \
		${NETBSDSRCDIR}/etc/netconfig ${DISTRIBDIR}/common/protocols \
		${DISTRIBDIR}/common/services
MAKEDEVTARGETS=	std md0 wd0 wd1 wd2 wd3 cd0 cd1 sd0 sd1 sd2 sd3 st0 ipty
MAKEDEVTARGETS+= ld0 ld1
MAKEDEVTARGETS+= drvctl dk0 dk1 dk2 dk3
MAKEDEVTARGETS+= flash0 flash1 flash2 flash3 flash4 flash5 flash6 flash7

# Use stubs to eliminate some large stuff from libc
HACKSRC=	${DISTRIBDIR}/utils/libhack
.include	"${HACKSRC}/Makefile.inc"
${CRUNCHBIN}:	libhack.o

.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

release:

.include <bsd.prog.mk>
