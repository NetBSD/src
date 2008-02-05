#	$NetBSD: Makefile.inc,v 1.8.2.2 2008/02/05 19:03:39 garbled Exp $
#
#	etc.ofppc/Makefile.inc -- ofppc-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC

BUILD_KERNELS+=		INSTALL
