#	$NetBSD: Makefile.inc,v 1.15 2002/05/07 14:11:37 isaki Exp $
#
#	etc.x68k/Makefile.inc -- x68k-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/floppy \
			installation/misc

LOCALTIME=		Japan

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib/notes && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/${MACHINE} && ${MAKE} dependall
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release
	cd ${.CURDIR}/../distrib/${MACHINE} && ${MAKE} release
	cd ${.CURDIR}/../sys/arch/x68k/stand/loadbsd && ${MAKE} release
	${MAKESUMS} -t ${RELEASEDIR}/installation/floppy '*.gz'
