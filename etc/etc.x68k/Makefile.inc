#	$NetBSD: Makefile.inc,v 1.6.2.1 2000/07/09 21:45:35 he Exp $
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
	cd ${.CURDIR}/../distrib/${MACHINE}/floppies && ${MAKE} clean
.endif
	cd ${.CURDIR}/../distrib/${MACHINE}/floppies && \
		${MAKE} depend && ${MAKE} ${_J} && ${MAKE} release
