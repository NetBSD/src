#	$NetBSD: Makefile.inc,v 1.9 2000/11/28 03:18:18 minoura Exp $
#
#	etc.x68k/Makefile.inc -- x68k-specific etc Makefile targets
#

# Set this for any kernels beyond GENERIC you want to include
# in the distribution.
BUILD_KERNELS+=	INSTALL

LOCALTIME=	Japan

INSTALLATION_DIRS+=	installation/floppy	\
			installation/misc

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/${MACHINE} && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release
