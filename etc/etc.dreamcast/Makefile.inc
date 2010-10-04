#	$NetBSD: Makefile.inc,v 1.4 2010/10/04 16:03:30 tsutsui Exp $
#
#	etc.dreamcast/Makefile.inc -- dreamcast-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC
KERNEL_SUFFIXES=	bin

BUILD_KERNELS=		GENERIC_MD
