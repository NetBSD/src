#	$NetBSD: Makefile.inc,v 1.3 2002/03/30 06:55:17 thorpej Exp $
#
#	etc.shark/Makefile.inc -- shark-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/instkernel

# Build the release notes and boot floppies and install them
snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/shark && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release
	sh ../distrib/sets/makesums -t \
	    ${RELEASEDIR}/installation/instkernel '*.gz'
