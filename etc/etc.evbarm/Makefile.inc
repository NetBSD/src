#	$NetBSD: Makefile.inc,v 1.11 2003/01/27 16:13:27 thorpej Exp $
#
#	etc.evbarm/Makefile.inc -- evbarm-specific etc Makefile targets
#

KERNEL_SETS=		ADI_BRH INTEGRATOR IQ80310 IQ80321 IXM1200 \
			SMDK2800 TEAMASA_NPWR
KERNEL_SUFFIXES=	bin srec

BUILD_KERNELS+=		TEAMASA_NPWR_INSTALL

INSTALLATION_DIRS+=	binary/gzimg		\
			installation/instkernel
