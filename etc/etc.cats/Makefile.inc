#	$NetBSD: Makefile.inc,v 1.5 2002/04/11 16:33:58 skrll Exp $
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
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/cats && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release
	${MAKESUMS} -t ${RELEASEDIR}/installation/kernel '*.gz'
#	${MAKESUMS} -t ${RELEASEDIR}/installation/misc '*.gz'
