#	$NetBSD: Makefile.inc,v 1.21.2.3 2000/10/01 21:53:31 hubertf Exp $
#
#	etc.i386/Makefile.inc -- i386-specific etc Makefile targets
#

# These are packaged and distributed
EXTRA_KERNELS+= GENERIC_TINY

# These are for use by the bootfloppy construction tools
BUILD_KERNELS+= INSTALL INSTALL_SMALL INSTALL_TINY
INSTALLATION_DIRS += installation/misc

# mkisofs arguments to generate bootable iso image
MKISOFS_FLAGS+=	-b installation/floppy/boot-big.fs

# Build the boot floppies and install them
snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/i386 && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release
	cd ${.CURDIR}/../sys/arch/i386/stand/dosboot && ${MAKE} release
	sh ../distrib/sets/makesums -t ${RELEASEDIR}/installation/floppy '*.gz'

# For "make iso-image"
iso-image_md_pre:
	echo iso-image_md_pre ${MACHINE_ARCH}
iso-image_md_post:
	echo iso-image_md_post ${MACHINE_ARCH}
