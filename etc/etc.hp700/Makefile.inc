#	$NetBSD: Makefile.inc,v 1.5.36.1 2012/10/30 18:49:03 yamt Exp $
#
#	etc.hp700/Makefile.inc -- hp700-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		GENERIC.MP RAMDISK

INSTALLATION_DIRS+=     installation
