#	$NetBSD: Makefile.inc,v 1.13 2002/03/31 16:07:56 bjh21 Exp $
#
#	etc.x68k/Makefile.inc -- x68k-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		INSTALL

LOCALTIME=	Japan

INSTALLATION_DIRS+=	installation/floppy	\
			installation/misc

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
