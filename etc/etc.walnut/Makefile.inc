#	$NetBSD: Makefile.inc,v 1.3 2002/05/30 16:23:30 eeh Exp $
#
#	etc.walnut/Makefile.inc -- walnut-specific etc Makefile targets
#

KERNEL_SETS=		WALNUT
KERNEL_SUFFIXES=	img

BUILD_KERNELS=		INSTALL

snap_pre_md:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir 
.endif

#
# Install miniroot images and auxiliary scripts to the release tree
#
snap_md_post:
	# make boot/miniroot images
	(cd ${.CURDIR}/../distrib; ${MAKE})

	# transfer images to release hierarchy
	(cd ${.CURDIR}/../distrib/walnut; ${MAKE} release)
	(cd ${.CURDIR}/../distrib/notes; ${MAKE} release)
