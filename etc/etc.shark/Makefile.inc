#	$NetBSD: Makefile.inc,v 1.7.16.1 2007/03/22 20:10:52 jdc Exp $
#
#	etc.shark/Makefile.inc -- shark-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC
KERNEL_SUFFIXES=	aout

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/instkernel

snap_md_post:
	cd ${NETBSDSRCDIR}/distrib/shark/instkernel/instkernel && \
	    ${MAKE} release-aout-instkernel
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/instkernel '*.gz'
