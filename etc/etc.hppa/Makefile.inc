#	$NetBSD: Makefile.inc,v 1.1 2014/02/24 07:23:40 skrll Exp $
#
#	etc.hppa/Makefile.inc -- hppa-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		GENERIC.MP RAMDISK

INSTALLATION_DIRS+=     installation
