#	$NetBSD: Makefile,v 1.9 2008/01/23 19:52:24 bouyer Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

#	create ${RAMDISK_*} variables
#
RAMDISKS=	RAMDISK_B	ramdisk-big
# RAMDISKS+=	RAMDISK_S	ramdisk-small
# RAMDISKS+=	RAMDISK_T	ramdisk-tiny
# RAMDISKS+=	RESCUE_T	ramdisk-rescuetiny

.for V F in ${RAMDISKS}
${V}DIR!=	cd ${.CURDIR}/../ramdisks/${F} && ${PRINTOBJDIR}
${V}=		${${V}DIR}/${F}.fs
.endfor

# If you change the list of distributed install kernels, don't forget
# to update the release documentation in distrib/notes/common/contents
MDSETTARGETS=	INSTALL_LARGE		${RAMDISK_B}	-
MDSETTARGETS+=	INSTALL			${RAMDISK_B}	-
MDSETTARGETS+=	INSTALL_XEN2_DOMU	${RAMDISK_B}	-
MDSETTARGETS+=	INSTALL_XEN3_DOMU	${RAMDISK_B}	-
MDSETTARGETS+=	INSTALL_XEN3PAE_DOMU	${RAMDISK_B}	-
# MDSETTARGETS+=	INSTALL_SMALL		${RAMDISK_S}	-
# MDSETTARGETS+=	INSTALL_TINY		${RAMDISK_T}	-
# MDSETTARGETS+=	INSTALL_PS2		${RAMDISK_S}	-
# MDSETTARGETS+=	INSTALL_TINY		${RESCUE_T}	netbsd-RESCUE_TINY

MDSET_RELEASEDIR=	binary/kernel
# do not strip Xen kernels, there's no space constraints here.
MDSET_NOSTRIP.netbsd-INSTALL_XEN2_DOMU=
MDSET_NOSTRIP.netbsd-INSTALL_XEN3_DOMU=
MDSET_NOSTRIP.netbsd-INSTALL_XEN3PAE_DOMU=
# do not strip any other kernels, it is done by Makefile.tarfloppy before
# they are written to a floppy.
MDSET_NOSTRIP=
MDSET_NOSYMBOLS.netbsd-INSTALL_XEN2_DOMU=
MDSET_NOSYMBOLS.netbsd-INSTALL_XEN3_DOMU=
MDSET_NOSYMBOLS.netbsd-INSTALL_XEN3PAE_DOMU=

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
