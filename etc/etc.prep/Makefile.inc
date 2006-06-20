#	$NetBSD: Makefile.inc,v 1.7 2006/06/20 17:42:22 garbled Exp $
#
#	etc.prep/Makefile.inc -- prep-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		INSTALL INSTALL_SMALL

INSTALLATION_DIRS+=	installation/floppy

# mkisofs arguments to generate bootable iso image
MKISOFS_FLAGS+= -prep-boot installation/floppy/sysinst.fs

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/floppy '*'
