#	$NetBSD: Makefile.inc,v 1.6.14.2 2007/07/24 10:49:28 pavel Exp $
#
#	etc.newsmips/Makefile.inc -- newsmips-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/floppy

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/floppy '*.fs'
