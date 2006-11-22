#	$NetBSD: Makefile,v 1.12 2006/11/22 20:54:16 martin Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

.include "${.CURDIR}/../instfs/Makefile.instfs"
IMAGE=			install.fs
IMAGESIZE=		5m

IMAGETAR=		instfs.tgz
IMAGETAR_RELEASEDIR=	installation/misc

MDSETTARGETS=		INSTALL	${IMAGE} -
MDSET_RELEASEDIR=	binary/kernel

# Use stubs to eliminate some large stuff from libc
HACKSRC=	${DISTRIBDIR}/utils/libhack
.include	"${HACKSRC}/Makefile.inc"
${CRUNCHBIN}:	libhack.o

.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"
.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
