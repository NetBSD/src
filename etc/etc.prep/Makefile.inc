#	$NetBSD: Makefile.inc,v 1.3 2002/03/31 16:07:56 bjh21 Exp $
#
#	etc.prep/Makefile.inc -- prep-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		INSTALL

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
	${MAKESUMS} -t ${RELEASEDIR}/installation/floppy '*'
