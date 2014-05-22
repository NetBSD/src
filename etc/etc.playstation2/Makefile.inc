#	$NetBSD: Makefile.inc,v 1.7.4.1 2014/05/22 11:27:20 yamt Exp $
#
#	etc.playstation2/Makefile.inc --
#		playstation2-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC

BUILD_KERNELS=	 	RAMDISK

INSTALLATION_DIRS+=	installation/miniroot
