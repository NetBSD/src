#	$NetBSD: Makefile.inc,v 1.1.8.2 2014/08/19 23:45:50 tls Exp $
#
#	etc.hppa/Makefile.inc -- hppa-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		GENERIC.MP RAMDISK

INSTALLATION_DIRS+=     installation
