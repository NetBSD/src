#	$NetBSD: Makefile,v 1.16 2011/11/01 09:28:14 martin Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

.include "${.CURDIR}/../instfs/Makefile.instfs"
IMAGE=			install.fs
IMAGESIZE=		9m

IMAGETAR=		instfs.tgz
IMAGETAR_RELEASEDIR=	installation/misc

MDSETTARGETS=		INSTALL	${IMAGE} -
MDSET_RELEASEDIR=	binary/kernel

SMALLPROG_INET6=1
.if ${USE_INET6} != "no"
LISTS+=	${DISTRIBDIR}/common/list.inet6
.endif

# Use stubs to eliminate some large stuff from libc
HACKSRC=	${DISTRIBDIR}/utils/libhack
.include	"${HACKSRC}/Makefile.inc"
${CRUNCHBIN}:	libhack.o

.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.dhcpcd"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"
.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
