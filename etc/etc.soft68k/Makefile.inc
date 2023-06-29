#	$NetBSD: Makefile.inc,v 1.7 2014/11/13 16:18:38 tsutsui Exp $
#
#	etc.soft68k/Makefile.inc -- soft68k-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/netboot
