#	$NetBSD: Makefile.inc,v 1.11 2001/11/29 22:45:55 thorpej Exp $
#
#	etc.arm32/Makefile.inc -- arm32-specific etc Makefile targets
#

KERNEL_SETS=		SHARK

BUILD_KERNELS=		SHARKINST

INSTALLATION_DIRS+=	installation/kernel \
			installation/misc

# Build the release notes and boot floppies and install them
snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/arm32 && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release
	sh ../distrib/sets/makesums -t ${RELEASEDIR}/installation/kernels '*.gz'
	sh ../distrib/sets/makesums -t ${RELEASEDIR}/installation/misc '*.gz'
