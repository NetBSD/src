#	$NetBSD: Makefile.inc,v 1.9 2002/03/31 16:07:54 bjh21 Exp $
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
#	${MAKESUMS} -t ${RELEASEDIR}/installation/miniroot '*.gz'
