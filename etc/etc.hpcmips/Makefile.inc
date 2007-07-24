#	$NetBSD: Makefile.inc,v 1.9.18.2 2007/07/24 10:49:26 pavel Exp $
#
#	etc.hpcmips/Makefile.inc -- hpcmips-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC TX3912

BUILD_KERNELS=		RAMDISK INSTALL_TX3912

INSTALLATION_DIRS+=	installation/miniroot
