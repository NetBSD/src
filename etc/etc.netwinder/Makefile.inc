#	$NetBSD: Makefile.inc,v 1.3 2002/03/20 17:05:53 bjh21 Exp $
#
#	etc.netwinder/Makefile.inc -- netwinder-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

#BUILD_KERNELS=		INSTALL

#INSTALLATION_DIRS+=	installation/kernel installation/misc

# Build the release notes and boot floppies and install them
snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
#	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
#	cd ${.CURDIR}/../distrib/arm32 && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release
#	sh ../distrib/sets/makesums -t ${RELEASEDIR}/installation/kernel '*.gz'
#	sh ../distrib/sets/makesums -t ${RELEASEDIR}/installation/misc '*.gz'
