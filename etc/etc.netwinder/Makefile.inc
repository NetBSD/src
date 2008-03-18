#	$NetBSD: Makefile.inc,v 1.9 2008/03/18 04:20:37 lukem Exp $
#
#	etc.netwinder/Makefile.inc -- netwinder-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC

#BUILD_KERNELS=		INSTALL

#INSTALLATION_DIRS+=	installation/kernel	\
#			installation/misc

# Build the release notes and boot floppies and install them
snap_md_post:
#	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/kernel '*.gz'
#	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc '*.gz'
