#	$NetBSD: Makefile.inc,v 1.1.4.2 2014/05/22 11:27:20 yamt Exp $
#
#	etc.hppa/Makefile.inc -- hppa-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		GENERIC.MP RAMDISK

INSTALLATION_DIRS+=     installation
