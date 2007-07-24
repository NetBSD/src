#	$NetBSD: Makefile.inc,v 1.9.14.2 2007/07/24 10:49:29 pavel Exp $
#
#	etc.shark/Makefile.inc -- shark-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC
KERNEL_SUFFIXES=	aout

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/instkernel

snap_md_post:
	cd ${NETBSDSRCDIR}/distrib/shark/instkernel/instkernel && \
	    ${MAKE} release-aout-instkernel
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/instkernel '*.gz'
