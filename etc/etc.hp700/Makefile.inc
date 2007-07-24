#	$NetBSD: Makefile.inc,v 1.5.18.2 2007/07/24 10:49:26 pavel Exp $
#
#	etc.hp700/Makefile.inc -- hp700-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		RAMDISK

INSTALLATION_DIRS+=     installation
