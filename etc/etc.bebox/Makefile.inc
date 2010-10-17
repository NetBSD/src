#	$NetBSD: Makefile.inc,v 1.6 2010/10/17 10:56:48 kiyohara Exp $
#
#	etc.bebox/Makefile.inc -- bebox-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/floppy

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/floppy '*'
