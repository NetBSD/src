#	$NetBSD: Makefile.inc,v 1.10 2002/02/10 01:09:54 gmcgarry Exp $
#
#	etc.hp300/Makefile.inc -- hp300-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

EXTRA_KERNELS=		INSTALL RAMDISK

# Build the boot media and install notes and install them
snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
	cd ${.CURDIR}/../sys/arch/hp300/stand && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/miniroot && ${MAKE} release
	cd ${.CURDIR}/../sys/arch/hp300/stand && \
	  ${MAKE} depend && ${MAKE} && ${MAKE} release
	sh ${.CURDIR}/../distrib/sets/makesums -t ${RELEASEDIR}/installation/misc 'SYS*'
