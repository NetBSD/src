#	$NetBSD: Makefile.inc,v 1.7 2008/01/23 23:18:50 garbled Exp $
#
#	etc.ofppc/Makefile.inc -- ofppc-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC GENERIC_B64

BUILD_KERNELS+=		INSTALL INSTALL_B64
