#	$NetBSD: Makefile.inc,v 1.3 2003/10/18 03:00:05 matt Exp $
#
#	etc.amd64/Makefile.inc -- amd64-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC GENERIC.MP

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+= 	installation/floppy

# mkisofs arguments to generate bootable iso image
MKISOFS_FLAGS+=	-b ${MACHINE}/installation/floppy/boot-big.fs

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/floppy '*.fs'
