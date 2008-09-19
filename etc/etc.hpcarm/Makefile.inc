#	$NetBSD: Makefile.inc,v 1.8 2008/09/19 17:38:43 tsutsui Exp $
#
#	etc.hpcarm/Makefile.inc -- hpcarm-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		IPAQ JORNADA720

BUILD_KERNELS=		INSTALL_IPAQ

INSTALLATION_DIRS+=	installation/miniroot

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation \
	    '*.gz' '*.exe'
