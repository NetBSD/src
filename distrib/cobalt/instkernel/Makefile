#	$NetBSD: Makefile,v 1.1.2.2 2008/02/02 14:06:49 tsutsui Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		RAMDISK ${RAMDISK} -
MDSET_RELEASEDIR=	binary/kernel

# do not strip kernels, there's no space constraints here.
MDSET_NOSTRIP.netbsd-RAMDISK=
MDSET_NOSYMBOLS.netbsd-RAMDISK=

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
