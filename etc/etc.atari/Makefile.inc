#	$NetBSD: Makefile.inc,v 1.8 2001/11/29 22:45:55 thorpej Exp $
#
#	etc.atari/Makefile.inc -- atari-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

EXTRA_KERNELS=		ATARITT BOOT BOOTX FALCON HADES

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
