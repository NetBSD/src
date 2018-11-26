#	$NetBSD: Makefile.inc,v 1.9.22.1 2018/11/26 01:49:59 pgoyette Exp $
#
#	etc.macppc/Makefile.inc -- macppc-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC GENERIC.MP GENERIC_601

BUILD_KERNELS+=		INSTALL INSTALL_601 GENERIC_MD

INSTALLATION_DIRS+=	installation/floppy
