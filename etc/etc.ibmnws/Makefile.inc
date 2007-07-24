#	$NetBSD: Makefile.inc,v 1.3 2007/07/24 10:49:25 pavel Exp $
#
#	etc.ibmnws/Makefile.inc -- ibmnws-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=            GENERIC

BUILD_KERNELS=

INSTALLATION_DIRS+=     installation/netboot
