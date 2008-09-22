#	$NetBSD: Makefile.inc,v 1.3.20.1 2008/09/22 18:15:00 bouyer Exp $
#
#	etc.hpcsh/Makefile.inc -- hpcsh-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC HPW650PA

BUILD_KERNELS=		RAMDISK

INSTALLATION_DIRS+=	installation/miniroot

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation \
	    '*.gz' '*.exe'
