#	$NetBSD: Makefile.inc,v 1.7.4.1 2000/07/14 15:44:20 minoura Exp $
#
#	etc.x68k/Makefile.inc -- x68k-specific etc Makefile targets
#

# Set this for any kernels beyond GENERIC you want to include
# in the distribution.
BUILD_KERNELS+=	INSTALL INSTALL_SMALL

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
