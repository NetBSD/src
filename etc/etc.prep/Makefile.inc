#	$NetBSD: Makefile.inc,v 1.4 2002/05/02 15:09:47 lukem Exp $
#
#	etc.prep/Makefile.inc -- prep-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/floppy

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
