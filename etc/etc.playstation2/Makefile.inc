#	$NetBSD: Makefile.inc,v 1.8.6.2 2014/08/19 23:45:51 tls Exp $
#
#	etc.playstation2/Makefile.inc --
#		playstation2-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC

BUILD_KERNELS=	 	RAMDISK

INSTALLATION_DIRS+=	installation/miniroot
