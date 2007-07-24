#	$NetBSD: Makefile.inc,v 1.15.14.2 2007/07/24 10:49:25 pavel Exp $
#
#	etc.cats/Makefile.inc -- cats-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC GENERIC.ABLE
KERNEL_SUFFIXES=	aout

BUILD_KERNELS=		INSTALL INSTALL.ABLE

INSTALLATION_DIRS+=	installation/kernel

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/kernel '*.gz'

iso-image-md-pre:
	gzip -dc ${RELEASEDIR}/${MACHINE}/binary/kernel/netbsd.aout-INSTALL.gz \
	    > ${CDROM.dir}/netbsd
