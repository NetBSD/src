#	$NetBSD: Makefile.inc,v 1.7.2.1 2002/06/28 13:51:31 lukem Exp $
#
#	etc.cats/Makefile.inc -- cats-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC
KERNEL_SUFFIXES=	aout

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/kernel

# Build the release notes and boot floppies and install them
snap_md_post:
.ifndef UPDATE
	cd ${.CURDIR}/../distrib && ${MAKE} cleandir
.endif
	cd ${.CURDIR}/../distrib && ${MAKE} depend && ${MAKE}
	cd ${.CURDIR}/../distrib/cats && ${MAKE} release
	cd ${.CURDIR}/../distrib/notes && ${MAKE} release
	${MAKESUMS} -t ${RELEASEDIR}/installation/kernel '*.gz'

iso-image_md_pre:
	cp -f ${RELEASEDIR}/binary/kernel/netbsd.aout-INSTALL.gz \
	    ${RELEASEDIR}/netbsd.gz
	gunzip ${RELEASEDIR}/netbsd.gz
