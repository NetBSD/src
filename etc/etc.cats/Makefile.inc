#	$NetBSD: Makefile.inc,v 1.10 2003/01/03 15:34:37 lukem Exp $
#
#	etc.cats/Makefile.inc -- cats-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC
KERNEL_SUFFIXES=	aout

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/kernel

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/kernel '*.gz'

iso-image_md_pre:
	cp -f ${RELEASEDIR}/${MACHINE}/binary/kernel/netbsd.aout-INSTALL.gz \
	    ${RELEASEDIR}/${MACHINE}/netbsd.gz
	gunzip ${RELEASEDIR}/${MACHINE}/netbsd.gz
