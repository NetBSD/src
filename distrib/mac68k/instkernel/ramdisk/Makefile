#	$NetBSD: Makefile,v 1.22 2002/04/11 15:46:33 lukem Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

IMAGE=		ramdisk.fs
IMAGESIZE=	2048k
MAKEFS_FLAGS=	-f 15

WARNS=		1
DBG=		-Os

CRUNCHBIN=	ramdiskbin
LISTS=		${.CURDIR}/list ${DISTRIBDIR}/common/list.sysinst
MTREECONF=	${DISTRIBDIR}/common/mtree.common
IMAGEENDIAN=	be
MAKEDEVTARGETS=	raminst
IMAGEDEPENDS=	${CRUNCHBIN} \
		disktab.preinstall dot.hdprofile dot.profile \
		${.CURDIR}/../../miniroot/termcap \
		${_SRC_TOP_}/etc/group ${_SRC_TOP_}/etc/master.passwd \
		${_SRC_TOP_}/etc/netconfig ${_SRC_TOP_}/etc/protocols \
		${_SRC_TOP_}/etc/services

.ifndef NOTESOBJDIR
NOTESOBJDIR!=	cd ${DISTRIBDIR}/notes/mac68k && ${PRINTOBJDIR}
.endif
IMAGEDEPENDS+=	${NOTESOBJDIR}/INSTALL.more
PARSELISTENV=	NOTESOBJDIR=${NOTESOBJDIR}

# Use stubs to eliminate some large stuff from libc
HACKSRC=	${DISTRIBDIR}/utils/libhack
.include	"${HACKSRC}/Makefile.inc"
${CRUNCHBIN}:	libhack.o


realall: ${IMAGE}

release:

.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

.include <bsd.prog.mk>
