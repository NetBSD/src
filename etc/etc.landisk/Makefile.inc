# $NetBSD: Makefile.inc,v 1.2 2007/05/02 08:02:00 skrll Exp $
#
# etc.landisk/Makefile.inc -- landisk-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC

BUILD_KERNELS+=		INSTALL

MD_INSTALLATION_DIRS=	installation/misc
INSTALLATION_DIRS+=	${MD_INSTALLATION_DIRS}
