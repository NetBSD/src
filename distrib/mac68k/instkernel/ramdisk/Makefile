#	$NetBSD: Makefile,v 1.15 2002/03/23 16:38:50 shiba Exp $

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
		dot.profile dot.hdprofile \
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

# Turn off libhack's gethostby*() as it only supports /etc/hosts
HACKOBJS:=	${HACKOBJS:Ngethost.o}

# This is listed in ramdiskbin.conf but is built here.
${CRUNCHBIN}: libhack.o


.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

PARSELISTENV+=	NOTESOBJDIR=${NOTESOBJDIR}

.include <bsd.prog.mk>
