#	$NetBSD: Makefile.inc,v 1.1 2001/08/22 18:34:48 matt Exp $
#
#	etc.netwinder/Makefile.inc -- netwinder-specific etc Makefile targets
#

# Set this for any kernels beyond GENERIC you want to include
# in the distribution.
#BUILD_KERNELS=	INSTALL

#INSTALLATION_DIRS+=	installation/kernel installation/misc

# Build the release notes and boot floppies and install them
snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
#	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
#	cd ${.CURDIR}/../distrib/arm32 && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release
#	sh ../distrib/sets/makesums -t ${RELEASEDIR}/installation/kernels '*.gz'
#	sh ../distrib/sets/makesums -t ${RELEASEDIR}/installation/misc '*.gz'
