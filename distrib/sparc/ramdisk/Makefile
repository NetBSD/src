#	$NetBSD: Makefile,v 1.36 2002/04/12 14:56:38 lukem Exp $
#
# ramdisk.fs is the microroot filesystem intended for use with
# the INSTALL kernel. It provides just enough tools to extract the
# miniroot-based `instfs' filesystem into an MFS mount point
# and start the installation scripts from there (see dot.profile).
#

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

IMAGE=		ramdisk.fs
IMAGESIZE=	1360b

WARNS=		1
DBG=		-Os

CRUNCHBIN=	ramdiskbin
LISTS=		${.CURDIR}/list
MTREECONF=	${DISTRIBDIR}/common/mtree.common
IMAGEENDIAN=	be
MAKEDEVTARGETS=	floppy
IMAGEDEPENDS+=	${CRUNCHBIN} dot.profile

# Use stubs to eliminate some large stuff from libc
HACKSRC=	${DISTRIBDIR}/utils/libhack
HACKOBJS=	gethost.o
.include	"${HACKSRC}/Makefile.inc"
${CRUNCHBIN}:	libhack.o


realall: ${IMAGE}

release:


.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

.include <bsd.prog.mk>
