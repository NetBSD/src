#	$NetBSD: Makefile.inc,v 1.8 2007/07/24 10:49:27 pavel Exp $
#
#	etc.prep/Makefile.inc -- prep-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		INSTALL INSTALL_SMALL

INSTALLATION_DIRS+=	installation/floppy

# mkisofs arguments to generate bootable iso image
MKISOFS_FLAGS+= -prep-boot installation/floppy/sysinst.fs

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/floppy '*'
