#	$NetBSD: Makefile.inc,v 1.6 2002/12/02 13:18:12 lukem Exp $
#
#	etc.shark/Makefile.inc -- shark-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC
KERNEL_SUFFIXES=	aout

BUILD_KERNELS=		INSTALL

INSTALLATION_DIRS+=	installation/instkernel

snap_md_post:
	${MAKESUMS} -t ${RELEASEDIR}/installation/instkernel '*.gz'
