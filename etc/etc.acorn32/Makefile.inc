#	$NetBSD: Makefile.inc,v 1.5 2002/03/31 16:07:54 bjh21 Exp $
#
#	etc.acorn32/Makefile.inc -- acorn32-specific etc Makefile targets
#

KERNEL_SETS=	GENERIC NC NC_WSCONS RPC_WSCONS
BUILD_KERNELS=	INSTALL

INSTALLATION_DIRS+=	installation/kernel \
			installation/misc

# Build the release notes and boot floppies and install them
snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/acorn32 && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release
	${MAKESUMS} -t ${RELEASEDIR}/installation/kernel '*.gz'
	${MAKESUMS} -t ${RELEASEDIR}/installation/misc '*.gz'
