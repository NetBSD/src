#	$NetBSD: Makefile.inc,v 1.19 2000/03/29 23:47:53 fredb Exp $
#
#	etc.i386/Makefile.inc -- i386-specific etc Makefile targets
#

# These are packaged and distributed
EXTRA_KERNELS+= GENERIC_TINY

# These are for use by the bootfloppy construction tools
BUILD_KERNELS+= INSTALL INSTALL_TINY

# Build the boot floppies and install them
snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib/i386/floppies && ${MAKE} clean
.endif
	cd ${.CURDIR}/../distrib/i386/floppies && \
		${MAKE} depend && ${MAKE} ${_J} && ${MAKE} release
	sh ../distrib/sets/makesums -t ${RELEASEDIR}/installation/floppies '*.gz'
