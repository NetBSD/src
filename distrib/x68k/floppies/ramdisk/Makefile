#	$NetBSD: Makefile,v 1.40.12.3 2014/08/19 23:45:49 tls Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

IMAGE=			inst.fs
IMAGESIZE=		1700k
# `floppy-root' cannot be used now, due to size issue.
#IMAGE_RELEASEDIR=	installation/floppy

MAKEFS_FLAGS=	-f 10
WARNS=		1
DBG=		-Os

CRUNCHBIN=	ramdiskbin
LISTS=		${.CURDIR}/list ${DISTRIBDIR}/common/list.sysinst.en
MTREECONF=	${DISTRIBDIR}/common/mtree.common
IMAGEENDIAN=	be
MAKEDEVTARGETS=	ramdisk
IMAGEDEPENDS+= 	${CRUNCHBIN} \
		dot.profile \
		${DISTRIBDIR}/common/services \
		${DESTDIR}/usr/mdec/boot ${DESTDIR}/usr/mdec/mboot \
		${DESTDIR}/usr/mdec/sdboot_ufs \
		${NETBSDSRCDIR}/etc/group ${NETBSDSRCDIR}/etc/master.passwd \
		${NETBSDSRCDIR}/etc/netconfig ${DISTRIBDIR}/common/protocols
SMALLPROG_INET6=1

.if ${USE_INET6} != "no"
LISTS+=		${DISTRIBDIR}/common/list.inet6
.endif

# Use stubs to eliminate some large stuff from libc
HACKSRC=	${DISTRIBDIR}/utils/libhack
.include	"${HACKSRC}/Makefile.inc"
${CRUNCHBIN}:	libhack.o

.include "${DISTRIBDIR}/common/Makefile.dhcpcd"
.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

release::

.include <bsd.prog.mk>
