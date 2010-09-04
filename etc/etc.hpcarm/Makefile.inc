#	$NetBSD: Makefile.inc,v 1.10 2010/09/04 01:16:50 tsutsui Exp $
#
#	etc.hpcarm/Makefile.inc -- hpcarm-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		IPAQ JORNADA720 WZERO3

BUILD_KERNELS=		INSTALL_IPAQ INSTALL_JORNADA720 INSTALL_WZERO3

INSTALLATION_DIRS+=	installation/miniroot

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation \
	    '*.gz' '*.exe'
