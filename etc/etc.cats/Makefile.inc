#	$NetBSD: Makefile.inc,v 1.14 2005/08/07 00:21:38 chris Exp $
#
#	etc.cats/Makefile.inc -- cats-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC GENERIC.ABLE
KERNEL_SUFFIXES=	aout

BUILD_KERNELS=		INSTALL INSTALL.ABLE

INSTALLATION_DIRS+=	installation/kernel

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/kernel '*.gz'

iso-image-md-pre:
	gzip -dc ${RELEASEDIR}/${MACHINE}/binary/kernel/netbsd.aout-INSTALL.gz \
	    > ${CDROM.dir}/netbsd
