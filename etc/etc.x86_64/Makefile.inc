#	$NetBSD: Makefile.inc,v 1.4 2003/03/07 00:48:48 fvdl Exp $
#
#	etc.x86_64/Makefile.inc -- x86_64-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC GENERIC.MP

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+= 	installation/floppy

# mkisofs arguments to generate bootable iso image
MKISOFS_FLAGS+=	-b installation/floppy/boot-big.fs

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/floppy '*.fs'
