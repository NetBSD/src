#	$NetBSD: Makefile.inc,v 1.12.2.3 2000/07/09 21:35:39 he Exp $
#
#	etc.i386/Makefile.inc -- i386-specific etc Makefile targets
#

# These are packaged and distributed
EXTRA_KERNELS+= GENERIC_TINY

# These are for use by the bootfloppy construction tools
BUILD_KERNELS= INSTALL INSTALL_TINY INSTALL_SMALL

# Build the boot floppies and install them
snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/i386 && ${MAKE} release
	sh ../distrib/sets/makesums -t ${RELEASEDIR}/installation/floppy '*.gz' '*.fs'
