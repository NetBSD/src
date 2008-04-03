#	$NetBSD: Makefile.inc,v 1.15.12.1 2008/04/03 13:54:11 mjf Exp $
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
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/kernel '*.gz'

iso-image-md-pre:
	gzip -dc ${RELEASEDIR}/${RELEASEMACHINEDIR}/binary/kernel/netbsd.aout-INSTALL.gz \
	    > ${CDROM.dir}/netbsd
