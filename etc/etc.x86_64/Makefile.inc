#	$NetBSD: Makefile.inc,v 1.1 2002/12/05 02:33:32 fvdl Exp $
#
#	etc.x86_64/Makefile.inc -- x86_64-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+= 	installation/floppy

# mkisofs arguments to generate bootable iso image
MKISOFS_FLAGS+=	-b installation/floppy/boot-big.fs

# Tar up extra kernel sets.
# Build the boot floppies and install them
snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/x86_64 && ${MAKE} release
#	cd ${.CURDIR}/../distrib/notes && ${MAKE} release
	${MAKESUMS} -t ${RELEASEDIR}/installation/floppy '*.fs'

# For "make iso-image"
iso-image_md_pre:
	echo iso-image_md_pre ${MACHINE_ARCH}
iso-image_md_post:
	echo iso-image_md_post ${MACHINE_ARCH}
