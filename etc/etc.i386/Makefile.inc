#	$NetBSD: Makefile.inc,v 1.33 2002/04/13 00:00:20 tv Exp $
#
#	etc.i386/Makefile.inc -- i386-specific etc Makefile targets
#

KERNEL_SETS=	GENERIC GENERIC_TINY GENERIC_LAPTOP GENERIC_DIAGNOSTIC \
		GENERIC_PS2TINY

BUILD_KERNELS=	INSTALL INSTALL_SMALL INSTALL_TINY INSTALL_LAPTOP INSTALL_PS2

INSTALLATION_DIRS += installation/misc

# mkisofs arguments to generate bootable iso image
MKISOFS_FLAGS+=	-b installation/floppy/boot-big.fs

# Tar up extra kernel sets.
# Build the boot floppies and install them
snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/i386 && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release
	cd ${.CURDIR}/../sys/arch/i386/stand/dosboot && ${MAKE} release
	cd ${.CURDIR}/../sys/arch/i386/stand/misc && ${MAKE} release
	${MAKESUMS} -t ${RELEASEDIR}/installation/floppy '*.fs'
	${MAKESUMS} -t ${RELEASEDIR}/installation/misc '*.*'

# For "make iso-image"
iso-image_md_pre:
	echo iso-image_md_pre ${MACHINE_ARCH}
iso-image_md_post:
	echo iso-image_md_post ${MACHINE_ARCH}
