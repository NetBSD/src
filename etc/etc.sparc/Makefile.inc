#	$NetBSD: Makefile.inc,v 1.13.2.1 2000/07/13 16:51:32 he Exp $
#
#	etc.sparc/Makefile.inc -- sparc-specific etc Makefile targets
#

# Set this for any kernels beyond GENERIC you want to include
# in the distribution.
EXTRA_KERNELS+= GENERIC_SCSI3

INSTALLATION_DIRS+= \
	installation/bootfs \
	installation/ramdisk

# Build the boot media and install them
snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/sparc && ${MAKE} release

