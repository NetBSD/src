#	$NetBSD: Makefile,v 1.16 2020/11/28 05:16:06 tsutsui Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

IMAGE=		ramdisk.fs
IMAGESIZE=	1440k
MAKEFS_FLAGS+=	-f 15

WARNS=		1
DBG=		-Os -fno-unwind-tables
DBG+=		-DNDEBUG		# to remove assert(3) macro

CRUNCHBIN=	ramdiskbin
LISTS=		${.CURDIR}/list # ${DISTRIBDIR}/common/list.sysinst
MTREECONF=	${DISTRIBDIR}/common/mtree.common
IMAGEENDIAN=	le
MAKEDEVTARGETS=	ramdisk
IMAGEDEPENDS=	${CRUNCHBIN} \
		dot.profile \
		${NETBSDSRCDIR}/etc/group ${NETBSDSRCDIR}/etc/master.passwd \
		${NETBSDSRCDIR}/etc/netconfig \
		${DISTRIBDIR}/common/protocols \
		${DISTRIBDIR}/common/services
#SMALLPROG_INET6=1

# Use stubs to eliminate some large stuff from libc
HACKSRC=	${DISTRIBDIR}/utils/libhack
.include	"${HACKSRC}/Makefile.inc"
${CRUNCHBIN}:	libhack.o

.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.dhcpcd"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

release:

.include <bsd.prog.mk>
