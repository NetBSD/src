#	$NetBSD: Makefile.inc,v 1.2 2002/12/06 00:14:11 lukem Exp $
#
#	etc.x86_64/Makefile.inc -- x86_64-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+= 	installation/floppy

# mkisofs arguments to generate bootable iso image
MKISOFS_FLAGS+=	-b installation/floppy/boot-big.fs

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/installation/floppy '*.fs'
