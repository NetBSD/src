#	$NetBSD: Makefile,v 1.6.26.1 2009/09/12 18:31:43 matt Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

.if ${MACHINE_ARCH} == "mipseb"
SFX=32
.else
SFX=64
.endif

IMAGE=		diskimage
IMAGESIZE=	3500k
LISTS=		${.CURDIR}/list${SFX}
MTREECONF=	${DISTRIBDIR}/common/mtree.dot
IMAGEENDIAN=	be
IMAGEDEPENDS=	netbsd-INSTALL${SFX}_IP2x.gz netbsd-INSTALL${SFX}_IP3x.gz 

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=	INSTALL${SFX}_IP2x ${RAMDISK} netbsd-INSTALL${SFX}_IP2x	\
		INSTALL${SFX}_IP3x ${RAMDISK} netbsd-INSTALL${SFX}_IP3x

MDSET_RELEASEDIR=	binary/kernel

.if ${MACHINE_ARCH} == "mipseb"
MDSET_SUFFIXES.netbsd-INSTALL32_IP2x=ecoff create-ecoff
create-ecoff=	${OBJCOPY} --impure -O ecoff-bigmips \
		-R .pdr -R .mdebug.abi32 -R .comment -R .ident \
		${.TARGET:R} ${.TARGET}
.endif

IMAGE_RELEASEDIR=	installation/diskimage

.include "${DISTRIBDIR}/common/Makefile.image"
.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
