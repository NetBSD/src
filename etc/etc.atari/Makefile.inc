#	$NetBSD: Makefile.inc,v 1.12 2002/05/30 21:00:05 leo Exp $
#
#	etc.atari/Makefile.inc -- atari-specific etc Makefile targets
#

KERNEL_SETS=		BOOT HADES MILAN-ISAIDE MILAN-PCIIDE

EXTRA_KERNELS=		ATARITT BOOTX FALCON

INSTALLATION_DIRS+=	installation/miniroot	\
			installation/misc
  
snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/miniroot && ${MAKE} && ${MAKE} release
	cd ${.CURDIR}/../distrib/atari && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release
#	${MAKESUMS} -t ${RELEASEDIR}/installation/miniroot '*.gz'
