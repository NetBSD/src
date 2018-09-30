#	$NetBSD: Makefile.inc,v 1.27.32.1 2018/09/30 01:45:08 pgoyette Exp $
#
#	etc.amiga/Makefile.inc -- amiga-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC

EXTRA_KERNELS=		INSTALL WSCONS

INSTALLATION_DIRS+=	installation/miniroot	\
			installation/misc

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc '*'
#	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/miniroot '*.gz'
