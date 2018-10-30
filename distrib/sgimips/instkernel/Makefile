#	$NetBSD: Makefile,v 1.11.4.1 2018/10/30 19:02:51 martin Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

.if ${MACHINE_ARCH} == "mipseb"
NX=		32
SFX=		# none
LISTS=		${.CURDIR}/list
.else
NX=		64
SFX=		.elf32
LISTS=		${.CURDIR}/list64
.endif

IMAGE=		diskimage
IMAGESIZE=	4000k
MTREECONF=	${DISTRIBDIR}/common/mtree.dot
IMAGEENDIAN=	be

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

IMAGEDEPENDS=	netbsd-INSTALL${NX}_IP2x.gz netbsd-INSTALL${NX}_IP3x.gz 
MDSETTARGETS=	INSTALL${NX}_IP2x ${RAMDISK} netbsd-INSTALL${NX}_IP2x${SFX} \
		INSTALL${NX}_IP3x ${RAMDISK} netbsd-INSTALL${NX}_IP3x${SFX}

MDSET_RELEASEDIR=	binary/kernel

MDSET_SUFFIXES.netbsd-INSTALL${NX}_IP2x=ecoff create-ecoff
create-ecoff=	${ELF2ECOFF} ${.TARGET:R} ${.TARGET}

IMAGE_RELEASEDIR=	installation/diskimage

.include "${DISTRIBDIR}/common/Makefile.image"
.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
