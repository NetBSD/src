#	$NetBSD: Makefile.inc,v 1.9 2002/12/02 13:18:06 lukem Exp $
#
#	etc.cats/Makefile.inc -- cats-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC
KERNEL_SUFFIXES=	aout

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/kernel

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/installation/kernel '*.gz'

iso-image_md_pre:
	cp -f ${RELEASEDIR}/binary/kernel/netbsd.aout-INSTALL.gz \
	    ${RELEASEDIR}/netbsd.gz
	gunzip ${RELEASEDIR}/netbsd.gz
