#	$NetBSD: Makefile.inc,v 1.9.24.1 2019/06/10 21:42:43 christos Exp $
#
#	etc.macppc/Makefile.inc -- macppc-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC GENERIC.MP GENERIC_601 POWERMAC_G5 \
			POWERMAC_G5.MP

BUILD_KERNELS+=		INSTALL INSTALL_601 GENERIC_MD

INSTALLATION_DIRS+=	installation/floppy
