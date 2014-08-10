#	$NetBSD: Makefile.inc,v 1.8.56.1 2014/08/10 06:49:15 tls Exp $
#
#	etc.macppc/Makefile.inc -- macppc-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC GENERIC.MP GENERIC_601

BUILD_KERNELS+=		INSTALL GENERIC_MD

INSTALLATION_DIRS+=	installation/floppy
