#	$NetBSD: Makefile.inc,v 1.7 2003/01/03 15:34:39 lukem Exp $
#
#	etc.netwinder/Makefile.inc -- netwinder-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

#BUILD_KERNELS=		INSTALL

#INSTALLATION_DIRS+=	installation/kernel	\
#			installation/misc

# Build the release notes and boot floppies and install them
snap_md_post:
#	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/kernel '*.gz'
#	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/misc '*.gz'
