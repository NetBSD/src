#	$NetBSD: Makefile.inc,v 1.1 2001/01/16 16:53:57 nonaka Exp $
#
#	etc.prep/Makefile.inc -- prep-specific etc Makefile targets
#

BUILD_KERNELS+=		INSTALL
INSTALLATION_DIRS+=	installation

# mkisofs arguments to generate bootable iso image
MKISOFS_FLAGS+= -prep-boot installation/floppy/sysinst.fs

snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/prep && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release
	sh ../distrib/sets/makesums -t ${RELEASEDIR}/installation/floppy '*'
