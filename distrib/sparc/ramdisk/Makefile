#	$NetBSD: Makefile,v 1.32 2002/03/11 07:04:21 lukem Exp $
#
# ramdisk.fs is the microroot filesystem intended for use with
# the INSTALL kernel. It provides just enough tools to extract the
# miniroot-based `instfs' filesystem into an MFS mount point
# and start the installation scripts from there (see dot.profile).
#

.include "${.CURDIR}/../../Makefile.inc"
.include <bsd.own.mk>

IMAGE=		ramdisk.fs
IMAGESIZE=	1360b

WARNS=		1

CRUNCHBIN=	ramdiskbin
LISTS=		${.CURDIR}/list
MTREECONF=	${DISTRIBDIR}/common/mtree.common
IMAGEENDIAN=	be
MAKEDEVTARGETS=	floppy
IMAGEDEPENDS+=	${CRUNCHBIN} dot.profile


realall: ${IMAGE}

# Use stubs to eliminate some large stuff from libc
HACKSRC=${_SRC_TOP_}/distrib/utils/libhack
.include "${HACKSRC}/Makefile.inc"

# This is listed in ramdiskbin.conf but is built here.
${CRUNCHBIN}: libhack.o

release:


.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

.include <bsd.prog.mk>
