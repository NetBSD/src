#	$NetBSD: Makefile.inc,v 1.1.2.2 2013/07/23 21:07:22 riastradh Exp $
#
#	etc.evbcf/Makefile.inc -- evbcf-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		TWR54418

BUILD_KERNELS=		TWR54418_INSTALL

snap_md_post:
