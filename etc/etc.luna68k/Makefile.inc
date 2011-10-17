#	$NetBSD: Makefile.inc,v 1.6 2011/10/17 14:42:42 tsutsui Exp $
#
#	etc.luna68k/Makefile.inc -- luna68k-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC
KERNEL_SUFFIXES=	aout

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/netboot
