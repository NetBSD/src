#	$NetBSD: Makefile.inc,v 1.2 2001/03/26 10:27:15 tsutsui Exp $

# These are put in ${RELEASEDIR} by "make release" in distrib/sun3
# after it builds and patches in the installation ramdisk image.
BUILD_KERNELS+=		INSTALL

# These are just built and copied to the release area.
EXTRA_KERNELS+=		GENERIC_TINY

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/news68k && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release
