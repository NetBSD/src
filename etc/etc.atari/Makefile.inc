#	$NetBSD: Makefile.inc,v 1.7 2000/11/04 21:15:36 leo Exp $
#
#	etc.atari/Makefile.inc -- atari-specific etc Makefile targets
#

# Set this for any kernels beyond GENERIC you want to include
# in the distribution.
EXTRA_KERNELS+=	ATARITT BOOT BOOTX FALCON HADES

INSTALLATION_DIRS+=	installation/miniroot \
			installation/misc
  
snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/miniroot && ${MAKE} && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release
#	sh ${.CURDIR}/../distrib/sets/makesums -t ${RELEASEDIR}/installation/miniroot '*.gz'
