#	$NetBSD: Makefile.inc,v 1.6 2008/01/02 11:32:16 mrg Exp $
#
#	etc.ofppc/Makefile.inc -- ofppc-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC

BUILD_KERNELS+=		INSTALL
