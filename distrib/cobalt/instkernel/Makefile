#	$NetBSD: Makefile,v 1.3 2021/04/25 23:40:05 christos Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

.if !empty(MACHINE_ARCH:Mmips*64el)
SFX=		64
.endif

MDSETTARGETS=		RAMDISK${SFX} ${RAMDISK} -
MDSET_RELEASEDIR=	binary/kernel

# do not strip kernels, there's no space constraints here.
MDSET_NOSTRIP.netbsd-RAMDISK${SFX}=
MDSET_NOSYMBOLS.netbsd-RAMDISK${SFX}=

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
