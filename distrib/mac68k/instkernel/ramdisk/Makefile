#	$NetBSD: Makefile,v 1.17 2002/03/25 07:43:43 lukem Exp $

.include "${.CURDIR}/../../../Makefile.inc"
.include <bsd.own.mk>

.ifndef NOTESOBJDIR
NOTESOBJDIR!=cd ${DISTRIBDIR}/notes/mac68k && ${PRINTOBJDIR}
.endif

IMAGE=		ramdisk.fs
IMAGESIZE=	2048k

WARNS=		1

CRUNCHBIN=	ramdiskbin
LISTS=		${.CURDIR}/list ${DISTRIBDIR}/common/list.sysinst
MTREECONF=	${DISTRIBDIR}/common/mtree.common
IMAGEENDIAN=	be
MAKEDEVTARGETS=	raminst
IMAGEDEPENDS=	${CRUNCHBIN} \
		disktab.preinstall dot.profile dot.hdprofile \
		${.CURDIR}/../../miniroot/termcap \
		${NOTESOBJDIR}/INSTALL.more \
		${_SRC_TOP_}/etc/group \
		${_SRC_TOP_}/etc/master.passwd \
		${_SRC_TOP_}/etc/protocols \
		${_SRC_TOP_}/etc/netconfig \
		${_SRC_TOP_}/etc/services


realall: ${IMAGE}

release:


# Use stubs to eliminate some large stuff from libc
HACKSRC=${DISTRIBDIR}/utils/libhack
.include "${HACKSRC}/Makefile.inc"

# This is listed in ramdiskbin.conf but is built here.
${CRUNCHBIN}: libhack.o


.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

PARSELISTENV+=	NOTESOBJDIR=${NOTESOBJDIR}

.include <bsd.prog.mk>
