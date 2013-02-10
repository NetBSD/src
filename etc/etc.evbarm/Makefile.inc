#	$NetBSD: Makefile.inc,v 1.39 2013/02/10 14:26:32 christos Exp $
#
#	etc.evbarm/Makefile.inc -- evbarm-specific etc Makefile targets
#

MKIMAGE= ${NETBSDSRCDIR}/distrib/utils/embedded/mkimage

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents
EVBARM_BOARDS=

.if ${MACHINE_ARCH} == "arm"
# Little endian platforms (armv4 or pre-armv5t)
EVBARM_BOARDS+=		ADI_BRH CP3100 GEMINI GUMSTIX INTEGRATOR \
			IQ80310 IQ80321 MINI2440 MV2120 SMDK2410 \
			SMDK2800 TEAMASA_NPWR TS7200 TWINTAIL
.endif

.if ${MACHINE_ARCH} == "arm" || ${MACHINE_ARCH} == "earm" \
    || ${MACHINE_ARCH} == "earmhf"
# Little endian platforms with VFP
EVBARM_BOARDS+=		BCM5301X BEAGLEBOARD BEAGLEBOARDXM BEAGLEBONE TISDP2420
.endif
.if ${MACHINE_ARCH} == "arm" || ${MACHINE_ARCH} == "earm"
# Little endian platforms (armv5t+)
EVBARM_BOARDS+=		OPENRD SHEEVAPLUG 
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
EXTRA_KERNELS+=		RPI
RPI.rel=		${RELEASEDIR}/${RELEASEMACHINEDIR}
RPI.kernel=		${RPI.rel}/binary/kernel/netbsd-RPI.bin.gz
RPI.img=		${RPI.rel}/binary/gzimg/rpi.img.gz
# The first 3 tools are for mkimage, the last 3 for MAKEDEV
snap_md_post:
	TOOL_MAKEFS=${TOOL_MAKEFS} \
	TOOL_DISKLABEL=${TOOL_DISKLABEL} \
	TOOL_FDISK=${TOOL_FDISK} \
	TOOL_MKNOD=${TOOL_MKNOD} \
	TOOL_PAX=${TOOL_PAX} \
	TOOL_MTREE=${TOOL_MTREE} \
	${MKIMAGE} -x -h rpi -D ${DESTDIR} -K ${RPI.kernel} \
	    -S ${NETBSDSRCDIR} ${RPI.img}
.endif

.if ${MACHINE_ARCH} == "armeb"
# Big endian platforms.
KERNEL_SETS+=		IXM1200 NSLU2
.endif

KERNEL_SUFFIXES=	bin srec ub

INSTALLATION_DIRS+=	binary/gzimg		\
			installation/instkernel
