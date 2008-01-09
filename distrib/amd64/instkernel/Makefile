#	$NetBSD: Makefile,v 1.2.4.1 2008/01/09 01:27:51 matt Exp $

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
MDSETTARGETS+=INSTALL_XEN3_DOMU		${RAMDISK}	-

MDSET_RELEASEDIR=	binary/kernel

# do not strip Xen kernels, there's no space constraints here.
MDSET_NOSTRIP.netbsd-INSTALL_XEN3_DOMU=
MDSET_NOSYMBOLS.netbsd-INSTALL_XEN3_DOMU=

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
