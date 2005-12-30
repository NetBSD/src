#	$NetBSD: Makefile.inc,v 1.4 2005/12/30 11:31:36 rjs Exp $
#
#	etc.amd64/Makefile.inc -- amd64-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC GENERIC.MP GENERIC_ACPI

BUILD_KERNELS=		INSTALL INSTALL_ACPI

INSTALLATION_DIRS+= 	installation/floppy

# mkisofs arguments to generate bootable iso image
MKISOFS_FLAGS+=	-b ${MACHINE}/installation/floppy/boot-big.fs

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/floppy '*.fs'
