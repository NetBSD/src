#	$NetBSD: Makefile.inc,v 1.38 2003/01/04 15:49:31 lukem Exp $
#
#	etc.i386/Makefile.inc -- i386-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC GENERIC_TINY GENERIC_LAPTOP \
			GENERIC_DIAGNOSTIC GENERIC_PS2TINY GENERIC.MP

BUILD_KERNELS=		INSTALL INSTALL_SMALL INSTALL_TINY \
			INSTALL_LAPTOP INSTALL_PS2

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
MKISOFS_FLAGS+=	-b ${MACHINE}/installation/floppy/boot-big.fs -c boot.catalog
