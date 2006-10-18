#	$NetBSD: Makefile.inc,v 1.19 2006/10/18 10:13:58 he Exp $
#
#	etc.evbarm/Makefile.inc -- evbarm-specific etc Makefile targets
#

.if ${MACHINE_ARCH} == "arm"
# Little endian platforms
EVBARM_BOARDS=		ADI_BRH GUMSTIX INTEGRATOR IQ80310 IQ80321 \
			SMDK2410 SMDK2800 \
			TEAMASA_NPWR TS7200 TWINTAIL
.else
# Big endian platforms
EVBARM_BOARDS=		ADI_BRH
.endif

.for board in ${EVBARM_BOARDS}
KERNEL_SETS+=		${board}
BUILD_KERNELS+=		${board}_INSTALL
.endfor

.if ${MACHINE_ARCH} == "arm"
# Little endian platforms.
KERNEL_SETS+=		IXM1200 HDL_G
.else
# Big endian platforms.
KERNEL_SETS+=		IXM1200 NSLU2
.endif

KERNEL_SUFFIXES=	bin srec

INSTALLATION_DIRS+=	binary/gzimg		\
			installation/instkernel
