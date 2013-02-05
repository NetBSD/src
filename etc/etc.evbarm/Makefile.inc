#	$NetBSD: Makefile.inc,v 1.33 2013/02/05 07:34:07 matt Exp $
#
#	etc.evbarm/Makefile.inc -- evbarm-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents
EVBARM_BOARDS=

.if ${MACHINE_ARCH} == "arm"
# Little endian platforms (armv4 or pre-armv5t)
EVBARM_BOARDS+=		ADI_BRH BCM5301X CP3100 GEMINI GUMSTIX INTEGRATOR \
			IQ80310 IQ80321 MINI2440 MV2120 SMDK2410 \
			SMDK2800 TEAMASA_NPWR TS7200 TWINTAIL
.endif

.if ${MACHINE_ARCH} == "arm" || ${MACHINE_ARCH} == "earm" \
    || ${MACHINE_ARCH} == "earmhf"
# Little endian platforms (armv5t+)
EVBARM_BOARDS+=		BEAGLEBOARD OPENRD SHEEVAPLUG TISDP2420
.endif

.if ${MACHINE_ARCH} == "armeb"
# Big endian platforms
EVBARM_BOARDS+=		ADI_BRH
.endif

.for board in ${EVBARM_BOARDS}
KERNEL_SETS+=		${board}
BUILD_KERNELS+=		${board}_INSTALL
.endfor

.if ${MACHINE_ARCH} == "arm"
# Little endian platforms.
KERNEL_SETS+=		IXM1200 HDL_G
.endif

.if ${MACHINE_ARCH} == "arm" || ${MACHINE_ARCH} == "earm" \
    || ${MACHINE_ARCH} == "earmhf"
KERNEL_SETS+=		RPI
.endif

.if ${MACHINE_ARCH} == "armeb"
# Big endian platforms.
KERNEL_SETS+=		IXM1200 NSLU2
.endif

KERNEL_SUFFIXES=	bin srec ub

INSTALLATION_DIRS+=	binary/gzimg		\
			installation/instkernel
