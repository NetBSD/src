#	$NetBSD: Makefile.inc,v 1.4.18.2 2007/07/24 10:49:26 pavel Exp $
#
#	etc.hpcsh/Makefile.inc -- hpcsh-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC HPW650PA

BUILD_KERNELS=		RAMDISK

INSTALLATION_DIRS+=	installation/miniroot
