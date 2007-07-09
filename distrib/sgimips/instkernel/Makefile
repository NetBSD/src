#	$NetBSD: Makefile,v 1.6 2007/07/09 21:35:59 he Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

IMAGE=		diskimage
IMAGESIZE=	3500k
LISTS=		${.CURDIR}/list
MTREECONF=	${DISTRIBDIR}/common/mtree.dot
IMAGEENDIAN=	be
IMAGEDEPENDS=	netbsd-INSTALL32_IP2x.gz netbsd-INSTALL32_IP3x.gz 

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		INSTALL32_IP2x ${RAMDISK} netbsd-INSTALL32_IP2x	\
			INSTALL32_IP3x ${RAMDISK} netbsd-INSTALL32_IP3x

MDSET_RELEASEDIR=	binary/kernel

MDSET_SUFFIXES.netbsd-INSTALL32_IP2x=ecoff create-ecoff
create-ecoff=	${OBJCOPY} --impure -O ecoff-bigmips \
		-R .pdr -R .mdebug.abi32 -R .comment -R .ident \
		${.TARGET:R} ${.TARGET}

IMAGE_RELEASEDIR=	installation/diskimage

.include "${DISTRIBDIR}/common/Makefile.image"
.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
