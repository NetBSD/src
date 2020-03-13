#	$NetBSD: Makefile.inc,v 1.29 2020/03/13 10:01:07 is Exp $
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
	${TOOL_GZIP} -fd ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/miniroot/miniroot.fs.gz
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/miniroot '*.fs*'
