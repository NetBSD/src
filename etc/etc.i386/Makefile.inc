#	$NetBSD: Makefile.inc,v 1.47.2.1 2007/03/04 12:10:41 bouyer Exp $
#
#	etc.i386/Makefile.inc -- i386-specific etc Makefile targets
#

KERNEL_SETS=	GENERIC
KERNEL_SETS+=	GENERIC_LAPTOP
KERNEL_SETS+=	GENERIC_DIAGNOSTIC
KERNEL_SETS+=	GENERIC.MP
KERNEL_SETS+=	GENERIC.MPACPI
KERNEL_SETS+=	XEN2_DOM0
KERNEL_SETS+=	XEN2_DOMU
KERNEL_SETS+=	XEN3_DOM0
KERNEL_SETS+=	XEN3_DOMU
# KERNEL_SETS+=	GENERIC_TINY
# KERNEL_SETS+=	GENERIC_PS2TINY

BUILD_KERNELS=	ALL
BUILD_KERNELS+=	INSTALL
BUILD_KERNELS+=	INSTALL_LAPTOP
BUILD_KERNELS+=	INSTALL_XEN2_DOMU
BUILD_KERNELS+=	INSTALL_XEN3_DOMU
# BUILD_KERNELS+=	INSTALL_SMALL
# BUILD_KERNELS+=	INSTALL_TINY
# BUILD_KERNELS+=	INSTALL_PS2

INSTALLATION_DIRS+=	installation/misc
INSTALLATION_DIRS+=	installation/cdrom
INSTALLATION_DIRS+=	installation/floppy

# i386 specific distrib stuff
snap_md_post:
	cd ${KERNSRCDIR}/arch/i386/stand/dosboot && ${MAKE} release
	cd ${KERNSRCDIR}/arch/i386/stand/pxeboot && ${MAKE} release
	cd ${KERNSRCDIR}/arch/i386/stand/misc && ${MAKE} release
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/cdrom '*.iso'
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/floppy '*.fs'
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/misc '*.*'

# For "make iso-image"
# mkisofs arguments to generate bootable iso image
CDROM_BOOT_IMAGE?=	boot-big.fs

MKISOFS_FLAGS+=	-b ${MACHINE}/installation/floppy/${CDROM_BOOT_IMAGE} -c boot.catalog
