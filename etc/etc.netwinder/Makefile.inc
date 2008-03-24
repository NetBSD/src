#	$NetBSD: Makefile.inc,v 1.8.10.1 2008/03/24 07:14:41 keiichi Exp $
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
