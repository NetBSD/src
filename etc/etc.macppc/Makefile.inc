#	$NetBSD: Makefile.inc,v 1.9 2014/07/20 14:52:29 macallan Exp $
#
#	etc.macppc/Makefile.inc -- macppc-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC GENERIC.MP GENERIC_601

BUILD_KERNELS+=		INSTALL GENERIC_MD

INSTALLATION_DIRS+=	installation/floppy
