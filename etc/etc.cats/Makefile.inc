#	$NetBSD: Makefile.inc,v 1.3 2002/03/31 16:07:54 bjh21 Exp $
#
#	etc.cats/Makefile.inc -- cats-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/kernel \
			installation/misc

# Build the release notes and boot floppies and install them
snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
#	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
#	cd ${.CURDIR}/../distrib/cats && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release
#	${MAKESUMS} -t ${RELEASEDIR}/installation/kernel '*.gz'
#	${MAKESUMS} -t ${RELEASEDIR}/installation/misc '*.gz'
