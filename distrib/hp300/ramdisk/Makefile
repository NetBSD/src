#       $NetBSD: Makefile,v 1.2 2002/02/13 05:13:24 gmcgarry Exp $

.include "${.CURDIR}/../../Makefile.inc"
.include <bsd.own.mk>

IMAGE=		ramdisk.fs
IMAGESIZE=	1440k
IMAGEDEPENDS=	${DESTDIR}/usr/mdec/uboot.lif
USE_SYSINST=	yes

WARNS=		1

CRUNCHBIN=	ramdiskbin
LISTS=		${.CURDIR}/list
MTREECONF=	${DISTRIBDIR}/common/mtree.common
IMAGEENDIAN=	le
IMAGEDEPENDS+=	${CRUNCHBIN} dot.profile termcap.mini \
		${_SRC_TOP_}/etc/group ${_SRC_TOP_}/etc/master.passwd \
		${_SRC_TOP_}/etc/protocols ${_SRC_TOP_}/etc/netconfig \
		${_SRC_TOP_}/etc/services ${DESTDIR}/usr/mdec/uboot.lif

.if defined(USE_SYSINST)
LISTS+=		${DISTRIBDIR}/common/list.sysinst
.endif

realall: ${IMAGE}

# Use stubs to eliminate some large stuff from libc
HACKSRC=${_SRC_TOP_}/distrib/utils/libhack
.include "${HACKSRC}/Makefile.inc"
# turn off small gethostby* temporarily
HACKOBJS:=	${HACKOBJS:Ngethost.o}

# This is listed in ramdiskbin.conf but is built here.
${CRUNCHBIN}: libhack.o

release:

.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

.include <bsd.prog.mk>
