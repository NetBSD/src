#	$NetBSD: Makefile.inc,v 1.41.2.2 2006/06/13 22:28:22 tron Exp $
#
#	etc.i386/Makefile.inc -- i386-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC GENERIC_TINY GENERIC_LAPTOP \
			GENERIC_DIAGNOSTIC GENERIC_PS2TINY GENERIC.MP \
			GENERIC.MPACPI XEN2_DOM0 XEN2_DOMU XEN3_DOMU

BUILD_KERNELS=		INSTALL INSTALL_SMALL INSTALL_TINY \
			INSTALL_LAPTOP INSTALL_PS2 INSTALL_XEN2_DOMU \
			INSTALL_XEN3_DOMU

INSTALLATION_DIRS+=	installation/misc	\
			installation/floppy

# i386 specific distrib stuff
snap_md_post:
	cd ${KERNSRCDIR}/arch/i386/stand/dosboot && ${MAKE} release
	cd ${KERNSRCDIR}/arch/i386/stand/misc && ${MAKE} release
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/floppy '*.fs'
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/misc '*.*'

# For "make iso-image"
# mkisofs arguments to generate bootable iso image
CDROM_BOOT_IMAGE?=	boot-big.fs

MKISOFS_FLAGS+=	-b ${MACHINE}/installation/floppy/${CDROM_BOOT_IMAGE} -c boot.catalog
