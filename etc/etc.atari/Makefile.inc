#	$NetBSD: Makefile.inc,v 1.11.2.1 2002/06/01 22:37:19 tv Exp $
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
