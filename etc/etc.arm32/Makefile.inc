#	$NetBSD: Makefile.inc,v 1.6.8.2 2000/11/13 19:20:48 tv Exp $
#
#	etc.arm32/Makefile.inc -- arm32-specific etc Makefile targets
#

# Set this for any kernels beyond GENERIC you want to include
# in the distribution.
EXTRA_KERNELS+= A7000  CATS     EBSA285 RISCPC  SHARK
BUILD_KERNELS=	A7INST CATSINST         RPCINST SHARKINST

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
