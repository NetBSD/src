#	$NetBSD: Makefile.inc,v 1.7 2000/05/31 14:33:11 he Exp $
#
#	etc.hp300/Makefile.inc -- hp300-specific etc Makefile targets
#

# Set this for any kernels beyond GENERIC you want to include
# in the distribution.
EXTRA_KERNELS+= DISKLESS

# Build the boot media and install notes and install them
snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
	cd ${.CURDIR}/../sys/arch/hp300/stand && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/miniroot && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release
	cd ${.CURDIR}/../sys/arch/hp300/stand && ${MAKE} release
	sh ${.CURDIR}/../distrib/sets/makesums -t ${RELEASEDIR}/installation/misc 'SYS*'
