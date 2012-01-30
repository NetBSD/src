#	$NetBSD: Makefile.inc,v 1.25 2012/01/30 03:28:34 nisimura Exp $
#
#	etc.evbarm/Makefile.inc -- evbarm-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

.if ${MACHINE_ARCH} == "arm"
# Little endian platforms
EVBARM_BOARDS=		ADI_BRH CP3100 GEMINI GUMSTIX INTEGRATOR \
			IQ80310 IQ80321 SHEEVAPLUG MINI2440 SMDK2410
			SMDK2800 TEAMASA_NPWR TISDP2420 TS7200 TWINTAIL
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
