#	$NetBSD: Makefile.inc,v 1.6 2002/12/02 13:18:10 lukem Exp $
#
#	etc.netwinder/Makefile.inc -- netwinder-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

#BUILD_KERNELS=		INSTALL

#INSTALLATION_DIRS+=	installation/kernel	\
#			installation/misc

# Build the release notes and boot floppies and install them
snap_md_post:
#	${MAKESUMS} -t ${RELEASEDIR}/installation/kernel '*.gz'
#	${MAKESUMS} -t ${RELEASEDIR}/installation/misc '*.gz'
