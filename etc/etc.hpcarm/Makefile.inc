#	$NetBSD: Makefile.inc,v 1.9 2010/04/17 13:36:21 nonaka Exp $
#
#	etc.hpcarm/Makefile.inc -- hpcarm-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		IPAQ JORNADA720 WZERO3

BUILD_KERNELS=		INSTALL_IPAQ INSTALL_WZERO3

INSTALLATION_DIRS+=	installation/miniroot

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation \
	    '*.gz' '*.exe'
