#	$NetBSD: Makefile.inc,v 1.5.4.1 2008/01/09 01:29:53 matt Exp $
#
#	etc.ofppc/Makefile.inc -- ofppc-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC

BUILD_KERNELS+=		INSTALL
