#	$NetBSD: Makefile.inc,v 1.1 2001/11/20 16:41:44 soren Exp $
#
#	etc.sgimips/Makefile.inc -- sgimips-specific etc Makefile targets
#

# Set this for any kernels beyond GENERIC you want to include
# in the distribution.
EXTRA_KERNELS+=GENERIC_INDY

# Set this for any kernels beyond GENERIC and ${EXTRA_KERNELS} you
# need to build to make installation tools/media.
BUILD_KERNELS+=RAMDISK

# Add any extra release directories to be made
INSTALLATION_DIRS+=installation/miniroot installation/netboot

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/prep && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release
