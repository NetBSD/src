#	$NetBSD: Makefile.inc,v 1.5.4.1 2007/03/04 12:10:41 bouyer Exp $
#
#	etc.amd64/Makefile.inc -- amd64-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC GENERIC.MP GENERIC_ACPI

BUILD_KERNELS=		INSTALL INSTALL_ACPI

INSTALLATION_DIRS+= 	installation/cdrom
INSTALLATION_DIRS+= 	installation/floppy
INSTALLATION_DIRS+= 	installation/misc

# mkisofs arguments to generate bootable iso image
MKISOFS_FLAGS+=	-b ${MACHINE}/installation/floppy/boot-big.fs

snap_md_post:
	cd ${KERNSRCDIR}/arch/i386/stand/pxeboot && ${MAKE} release
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/cdrom '*.iso'
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/floppy '*.fs'
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/misc '*.*'
