#	$NetBSD: Makefile.inc,v 1.4.2.2 2008/02/02 14:07:40 tsutsui Exp $
#
#	etc.cobalt/Makefile.inc -- cobalt-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC INSTALL

BUILD_KERNELS=		RAMDISK
