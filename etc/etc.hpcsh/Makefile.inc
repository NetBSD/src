#	$NetBSD: Makefile.inc,v 1.4.12.1 2008/10/05 20:11:19 mjf Exp $
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
