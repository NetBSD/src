#	$NetBSD: Makefile.inc,v 1.6.2.2 2000/07/27 17:55:55 he Exp $
#
#	etc.x68k/Makefile.inc -- x68k-specific etc Makefile targets
#

# Set this for any kernels beyond GENERIC you want to include
# in the distribution.
EXTRA_KERNELS+= SMALL
BUILD_KERNELS+= INSTALL

LOCALTIME=	Japan

INSTALLATION_DIRS+=   installation/floppy     \
                      installation/misc

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/${MACHINE} && ${MAKE} release
	sh ../distrib/sets/makesums -t ${RELEASEDIR}/installation/floppy '*.gz' '*.fs'
