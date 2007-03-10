#	$NetBSD: Makefile.inc,v 1.8 2007/03/10 17:57:41 jmmv Exp $
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
