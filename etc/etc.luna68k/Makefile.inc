#	$NetBSD: Makefile.inc,v 1.5 2011/07/16 15:52:21 tsutsui Exp $
#
#	etc.luna68k/Makefile.inc -- luna68k-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/netboot
