#	$NetBSD: Makefile.inc,v 1.5 2002/05/02 15:09:45 lukem Exp $
#
#	etc.netwinder/Makefile.inc -- netwinder-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

#BUILD_KERNELS=		INSTALL

#INSTALLATION_DIRS+=	installation/kernel	\
#			installation/misc

# Build the release notes and boot floppies and install them
snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
#	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
#	cd ${.CURDIR}/../distrib/arm32 && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release
#	${MAKESUMS} -t ${RELEASEDIR}/installation/kernel '*.gz'
#	${MAKESUMS} -t ${RELEASEDIR}/installation/misc '*.gz'
