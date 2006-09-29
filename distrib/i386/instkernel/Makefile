#	$NetBSD: Makefile,v 1.4 2006/09/29 08:20:44 pavel Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

#	create ${RAMDISK_*} variables
#
RAMDISKS=	RAMDISK_B	ramdisk-big		\
		RAMDISK_S	ramdisk-small		\
		RAMDISK_T	ramdisk-tiny		\
		RESCUE_T	ramdisk-rescuetiny

.for V F in ${RAMDISKS}
${V}DIR!=	cd ${.CURDIR}/../ramdisks/${F} && ${PRINTOBJDIR}
${V}=		${${V}DIR}/${F}.fs
.endfor

MDSETTARGETS=		INSTALL			${RAMDISK_B}	-	\
			INSTALL_LAPTOP		${RAMDISK_B}	-	\
			INSTALL_XEN2_DOMU	${RAMDISK_B}	-	\
			INSTALL_XEN3_DOMU	${RAMDISK_B}	-	\
			INSTALL_SMALL		${RAMDISK_S}	-	\
			INSTALL_TINY		${RAMDISK_T}	-	\
			INSTALL_PS2		${RAMDISK_S}	-	\
			INSTALL_TINY		${RESCUE_T}	netbsd-RESCUE_TINY

MDSET_RELEASEDIR=	binary/kernel
# do not strip Xen kernels, there's no space constraints here.
MDSET_NOSTRIP.netbsd-INSTALL_XEN2_DOMU=
MDSET_NOSTRIP.netbsd-INSTALL_XEN3_DOMU=
# do not strip any other kernels, it is done by Makefile.tarfloppy before
# they are written to a floppy.
MDSET_NOSTRIP=
MDSET_NOSYMBOLS.netbsd-INSTALL_XEN2_DOMU=
MDSET_NOSYMBOLS.netbsd-INSTALL_XEN3_DOMU=

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
