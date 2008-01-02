#	$NetBSD: Makefile,v 1.1 2008/01/02 11:39:01 mrg Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

#	create ${RAMDISK_*} variables
#
RAMDISKS=	RAMDISK		ramdisk

.for V F in ${RAMDISKS}
${V}DIR!=	cd ${.CURDIR}/../ramdisks/${F} && ${PRINTOBJDIR}
${V}=		${${V}DIR}/${F}.fs
.endfor

MDSETTARGETS=		INSTALL		${RAMDISK}	-

MDSET_RELEASEDIR=	binary/kernel

# do not strip kernels, there's no space constraints here.
MDSET_NOSTRIP.netbsd-INSTALL=
MDSET_NOSYMBOLS.netbsd-INSTALL=

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
