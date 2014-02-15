#	$NetBSD: Makefile.inc,v 1.23.8.2 2014/02/15 19:10:29 matt Exp $
#
#	etc.evbarm/Makefile.inc -- evbarm-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

.if ${MACHINE_ARCH} == "arm"
# Little endian platforms
EVBARM_BOARDS=		ADI_BRH CP3100 GEMINI GUMSTIX INTEGRATOR \
			IQ80310 IQ80321 SMDK2410 SMDK2800 \
			TEAMASA_NPWR TISDP2420 TS7200 TWINTAIL
EVBARM_BOARDS+=		BCM5301X BCM56340
.elif ${MACHINE_ARCH:Marmv7*} != ""
EVBARM_BOARDS=		BCM5301X BCM56340
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
.elif ${MACHINE_ARCH} == "armeb"
# Big endian platforms.
KERNEL_SETS+=		IXM1200 NSLU2
.endif

KERNEL_SUFFIXES=	bin srec

INSTALLATION_DIRS+=	binary/gzimg		\
			installation/instkernel
