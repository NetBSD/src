#	$NetBSD: Makefile.inc,v 1.4 2002/03/31 03:39:55 thorpej Exp $
#
#	etc.shark/Makefile.inc -- shark-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC
KERNEL_SUFFIXES=	aout

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
