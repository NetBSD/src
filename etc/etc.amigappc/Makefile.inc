#	$NetBSD: Makefile.inc,v 1.3.10.1 2008/03/24 07:14:41 keiichi Exp $
#
#	etc.amiga/Makefile.inc -- amiga-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=			

EXTRA_KERNELS=	

INSTALLATION_DIRS+=	installation/miniroot	\
			installation/misc

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc '*'
#	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/miniroot '*.gz'
