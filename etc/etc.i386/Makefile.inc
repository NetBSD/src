#	$NetBSD: Makefile.inc,v 1.41 2005/03/11 20:55:10 bouyer Exp $
#
#	etc.i386/Makefile.inc -- i386-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC GENERIC_TINY GENERIC_LAPTOP \
			GENERIC_DIAGNOSTIC GENERIC_PS2TINY GENERIC.MP \
			GENERIC.MPACPI XEN0 XENU

BUILD_KERNELS=		INSTALL INSTALL_SMALL INSTALL_TINY \
			INSTALL_LAPTOP INSTALL_PS2 INSTALL_XENU

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
