#	$NetBSD: Makefile.inc,v 1.12 2003/04/10 02:22:06 thorpej Exp $
#
#	etc.evbarm/Makefile.inc -- evbarm-specific etc Makefile targets
#

.for board in ADI_BRH INTEGRATOR IQ80310 IQ80321 TEAMASA_NPWR
KERNEL_SETS+=		${board}
BUILD_KERNELS+=		${board}_INSTALL
.endfor

KERNEL_SETS+=		IXM1200 SMDK2800

KERNEL_SUFFIXES=	bin srec

INSTALLATION_DIRS+=	binary/gzimg		\
			installation/instkernel
