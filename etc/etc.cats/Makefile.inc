#	$NetBSD: Makefile.inc,v 1.13 2003/03/22 00:13:35 chris Exp $
#
#	etc.cats/Makefile.inc -- cats-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC
KERNEL_SUFFIXES=	aout

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/kernel

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/kernel '*.gz'

iso-image-md-pre:
	gzip -dc ${RELEASEDIR}/${MACHINE}/binary/kernel/netbsd.aout-INSTALL.gz \
	    > ${CDROM.dir}/netbsd
