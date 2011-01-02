#	$NetBSD: Makefile.inc,v 1.5 2011/01/02 08:49:33 tsutsui Exp $
#
#	etc.amigappc/Makefile.inc -- amigappc-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC

EXTRA_KERNELS=	

#INSTALLATION_DIRS+=	installation/miniroot	\
#			installation/misc

#snap_md_post:
#	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc '*'
#	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/miniroot '*.gz'
