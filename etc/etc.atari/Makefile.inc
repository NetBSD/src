#	$NetBSD: Makefile.inc,v 1.17.6.2 2009/01/26 03:30:53 snj Exp $
#
#	etc.atari/Makefile.inc -- atari-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		SMALL030 ATARITT FALCON HADES MILAN-ISAIDE MILAN-PCIIDE

INSTALLATION_DIRS+=	installation/miniroot	\
			installation/misc

snap_md_post:
#	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/miniroot '*.gz'
