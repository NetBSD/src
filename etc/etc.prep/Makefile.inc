#	$NetBSD: Makefile.inc,v 1.5 2002/12/02 13:18:11 lukem Exp $
#
#	etc.prep/Makefile.inc -- prep-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/floppy

# mkisofs arguments to generate bootable iso image
MKISOFS_FLAGS+= -prep-boot installation/floppy/sysinst.fs

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/installation/floppy '*'
