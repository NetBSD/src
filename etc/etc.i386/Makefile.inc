#	$NetBSD: Makefile.inc,v 1.36 2002/12/02 13:18:08 lukem Exp $
#
#	etc.i386/Makefile.inc -- i386-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC GENERIC_TINY GENERIC_LAPTOP \
			GENERIC_DIAGNOSTIC GENERIC_PS2TINY GENERIC.MP

BUILD_KERNELS=		INSTALL INSTALL_SMALL INSTALL_TINY \
			INSTALL_LAPTOP INSTALL_PS2

INSTALLATION_DIRS+=	installation/misc	\
			installation/floppy

# mkisofs arguments to generate bootable iso image
MKISOFS_FLAGS+=	-b installation/floppy/boot-big.fs

# i386 specific distrib stuff
snap_md_post:
	cd ${KERNSRCDIR}/arch/i386/stand/dosboot && ${MAKE} release
	cd ${KERNSRCDIR}/arch/i386/stand/misc && ${MAKE} release
	${MAKESUMS} -t ${RELEASEDIR}/installation/floppy '*.fs'
	${MAKESUMS} -t ${RELEASEDIR}/installation/misc '*.*'

# For "make iso-image"
iso-image_md_pre:
	echo iso-image_md_pre ${MACHINE_ARCH}
iso-image_md_post:
	echo iso-image_md_post ${MACHINE_ARCH}
