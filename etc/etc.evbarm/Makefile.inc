#	$NetBSD: Makefile.inc,v 1.10 2002/12/02 13:18:07 lukem Exp $
#
#	etc.evbarm/Makefile.inc -- evbarm-specific etc Makefile targets
#

KERNEL_SETS=		INTEGRATOR IQ80310 IQ80321 TEAMASA_NPWR
KERNEL_SUFFIXES=	bin srec

BUILD_KERNELS+=		TEAMASA_NPWR_INSTALL

INSTALLATION_DIRS+=	binary/gzimg		\
			installation/instkernel
