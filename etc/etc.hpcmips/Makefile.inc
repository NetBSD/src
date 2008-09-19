#	$NetBSD: Makefile.inc,v 1.10 2008/09/19 17:38:43 tsutsui Exp $
#
#	etc.hpcmips/Makefile.inc -- hpcmips-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC TX3912

BUILD_KERNELS=		RAMDISK INSTALL_TX3912

INSTALLATION_DIRS+=	installation/miniroot

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation \
	    '*.gz' '*.exe'
