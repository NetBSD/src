#	$NetBSD: Makefile.inc,v 1.1 2009/04/30 12:47:40 tsutsui Exp $
#
#	etc.rs6000/Makefile.inc -- rs6000-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC

BUILD_KERNELS+=		INSTALL

MD_INSTALLATION_DIRS+=	installation/bootfs
INSTALLATION_DIRS+=	${MD_INSTALLATION_DIRS}

snap_md_post:
	# compute checksums
.for dir in ${MD_INSTALLATION_DIRS}
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/${dir} '*'
.endfor
