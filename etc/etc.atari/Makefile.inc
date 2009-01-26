#	$NetBSD: Makefile.inc,v 1.17.6.1 2009/01/26 03:21:05 snj Exp $
#
#	etc.atari/Makefile.inc -- atari-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		SMALL030 HADES MILAN-ISAIDE MILAN-PCIIDE

EXTRA_KERNELS=		ATARITT FALCON

INSTALLATION_DIRS+=	installation/miniroot	\
			installation/misc

snap_md_post:
#	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/miniroot '*.gz'
