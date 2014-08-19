#	$NetBSD: Makefile.inc,v 1.31.2.4 2014/08/19 23:45:50 tls Exp $
#
#	etc.evbarm/Makefile.inc -- evbarm-specific etc Makefile targets
#

MKIMAGE= ${NETBSDSRCDIR}/distrib/utils/embedded/mkimage

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents
EVBARM_BOARDS=
.for i in armv4 xscale armv5t armv6 armv7
EVBARM_BOARDS.${i}=
.endfor

.if !empty(MACHINE_ARCH:M*eb)
# big endian boards
KERNEL_SETS.armv4+=		IXM1200
EVBARM_BOARDS.xscale+=		ADI_BRH
KERNEL_SETS.xscale+=		NSLU2
EVBARM_BOARDS.armv7+=		BCM5301X
EVBARM_BOARDS.armv7hf+=		BCM5301X
#EVBARM_BOARDS.armv7+=		BCM56340
#EVBARM_BOARDS.armv7hf+= 	BCM56340
EVBARM_BOARDS.armv7+=		CUBIEBOARD
EVBARM_BOARDS.armv7hf+=		CUBIEBOARD
.else
# little endian boards
#EVBARM_BOARDS.armv4+=		ARMADILLO210
#EVBARM_BOARDS.armv4+=		ARMADILLO9
EVBARM_BOARDS.armv4+=		GEMINI
EVBARM_BOARDS.armv4+=		INTEGRATOR
KERNEL_SETS.armv4+=		INTEGRATOR_CP
KERNEL_SETS.armv4+=		IXM1200
#EVBARM_BOARDS.armv4+=		MINI2440
#EVBARM_BOARDS.armv4+=		MPCSA_GENERIC
#EVBARM_BOARDS.armv4+=		NAPPI
EVBARM_BOARDS.armv4+=		SMDK2410
EVBARM_BOARDS.armv4+=		SMDK2800
EVBARM_BOARDS.armv4+=		TS7200
EVBARM_BOARDS.xscale+=		ADI_BRH
EVBARM_BOARDS.xscale+=		CP3100
EVBARM_BOARDS.xscale+=		GUMSTIX
KERNEL_SETS.xscale+=		HDL_G
EVBARM_BOARDS.xscale+=		IQ80310
EVBARM_BOARDS.xscale+=		IQ80321
#EVBARM_BOARDS.xscale+=		LUBBOCK
EVBARM_BOARDS.xscale+=		TEAMASA_NPWR
EVBARM_BOARDS.xscale+=		TWINTAIL
#EVBARM_BOARDS.xscale+=		VIPER
#EVBARM_BOARDS.xscale+=		ZAO425
#EVBARM_BOARDS.armv5t+=		DNS323
#EVBARM_BOARDS.armv5t+=		HPT5325
#EVBARM_BOARDS.armv5t+=		IMX21_OLINUXINO
EVBARM_BOARDS.armv5t+=		KUROBOX_PRO
#EVBARM_BOARDS.armv5t+=		MARVELL_NAS
#EVBARM_BOARDS.armv5t+=		MMNET_GENERIC
EVBARM_BOARDS.armv5t+=		MV2120
#EVBARM_BOARDS.armv5t+=		OPENBLOCKS_A6
EVBARM_BOARDS.armv5t+=		OPENRD
#EVBARM_BOARDS.armv5t+=		OSK5912
EVBARM_BOARDS.armv5t+=		SHEEVAPLUG
#EVBARM_BOARDS.armv6+=		IMX31LITE
#EVBARM_BOARDS.armv6hf+= 	IMX31LITE
EVBARM_BOARDS.armv6+=		RPI
EVBARM_BOARDS.armv6hf+=	 	RPI
EVBARM_BOARDS.armv6+=		TISDP2420
EVBARM_BOARDS.armv6hf+= 	TISDP2420
#EVBARM_BOARDS.armv6+=		TISDP2430
EVBARM_BOARDS.armv7+=		ARMADAXP
EVBARM_BOARDS.armv7hf+=		ARMADAXP
EVBARM_BOARDS.armv7+=		BCM5301X
EVBARM_BOARDS.armv7hf+= 	BCM5301X
#EVBARM_BOARDS.armv7+=		BCM56340
#EVBARM_BOARDS.armv7hf+= 	BCM56340
EVBARM_BOARDS.armv7+=		BEAGLEBOARD
EVBARM_BOARDS.armv7hf+= 	BEAGLEBOARD
EVBARM_BOARDS.armv7+=		BEAGLEBOARDXM
EVBARM_BOARDS.armv7hf+= 	BEAGLEBOARDXM
EVBARM_BOARDS.armv7+=		BEAGLEBONE
EVBARM_BOARDS.armv7hf+= 	BEAGLEBONE
EVBARM_BOARDS.armv7+=		CUBIEBOARD
EVBARM_BOARDS.armv7hf+= 	CUBIEBOARD
#EVBARM_BOARDS.armv7+=		IGEPV2
EVBARM_BOARDS.armv7+=		MIRABOX
EVBARM_BOARDS.armv7hf+=		MIRABOX
#EVBARM_BOARDS.armv7+=		N900
EVBARM_BOARDS.armv7+=		NETWALKER
EVBARM_BOARDS.armv7hf+=		NETWALKER
EVBARM_BOARDS.armv7+=		OMAP5EVM
EVBARM_BOARDS.armv7hf+=		OMAP5EVM
#EVBARM_BOARDS.armv7+=		OVERO
EVBARM_BOARDS.armv7+=		PANDABOARD
EVBARM_BOARDS.armv7hf+= 	PANDABOARD
EVBARM_BOARDS.armv7+=		KOBO
EVBARM_BOARDS.armv7hf+= 	KOBO
.endif

IMAGE.rel=	${RELEASEDIR}/${RELEASEMACHINEDIR}
IMAGE.dir=	${IMAGE.rel}/binary/gzimg
IMAGE.kern=	${IMAGE.rel}/binary/kernel
IMAGE.instk=	${IMAGE.rel}/installation/instkernel

__mkimage: .USE
	TOOL_MAKEFS=${TOOL_MAKEFS} \
	TOOL_DISKLABEL=${TOOL_DISKLABEL} \
	TOOL_FDISK=${TOOL_FDISK} \
	TOOL_GZIP=${TOOL_GZIP} \
	TOOL_MKNOD=${TOOL_MKNOD} \
	TOOL_PAX=${TOOL_PAX} \
	TOOL_MTREE=${TOOL_MTREE} \
	HOST_SH=${HOST_SH} \
	${HOST_SH} ${MKIMAGE} -x -h ${.TARGET:S/smp_//} -D ${DESTDIR} \
	    -K ${.ALLSRC} -S ${NETBSDSRCDIR} ${MKI_OPTS.${.TARGET}} \
	    ${IMAGE.dir}/${.TARGET:S/smp_//}.img.gz

ARCHES.arm=		armv4 xscale armv5t armv6 armv7
ARCHES.armeb=		armv4 xscale armv5t armv6
ARCHES.earm=		xscale armv5t armv6 armv7
ARCHES.earmeb=		xscale armv5t armv6
ARCHES.earmhf=		armv6hf armv7hf
ARCHES.earmhfeb=	armv6hf
ARCHES.earmv4=		armv4
ARCHES.earmv4eb=	armv4
ARCHES.earmv5=		xscale armv5t
ARCHES.earmv5eb=	xscale armv5t
ARCHES.earmv6=		armv6
ARCHES.earmv6hf=	armv6hf
ARCHES.earmv6eb=	armv6
ARCHES.earmv6hfeb=	armv6hf
ARCHES.earmv7=		armv7
ARCHES.earmv7hf=	armv7hf
ARCHES.earmv7eb=	armv7
ARCHES.earmv7hfeb=	armv7hf
.for arch in ${ARCHES.${MACHINE_ARCH}}
.for board in ${EVBARM_BOARDS.${arch}}
KERNEL_SETS+=		${board}
BUILD_KERNELS+=		${board}_INSTALL
.endfor
KERNEL_SETS+=		${KERNEL_SETS.${arch}}
.endfor

.if !empty(KERNEL_SETS:MBEAGLEBOARD) && !empty(KERNEL_SETS:MBEAGLEBONE)
smp_beagleboard: ${IMAGE.kern}/netbsd-BEAGLEBOARD.ub.gz __mkimage
#            XXX ${IMAGE.kern}/netbsd-BEAGLEBONE.ub.gz
.if empty(ALL_KERNELS) || (!empty(ALL_KERNELS:MBEAGLEBOARD) && !empty(ALL_KERNELS:MBEAGLEBONE))
SNAP_MD_POST_DEPS+=	smp_beagleboard
.endif
.endif

.if !empty(KERNEL_SETS:MRPI)
smp_rpi: ${IMAGE.kern}/netbsd-RPI.bin.gz __mkimage
.if empty(ALL_KERNELS) || !empty(ALL_KERNELS:MRPI)
SNAP_MD_POST_DEPS+=	smp_rpi
.endif
MKI_OPTS.smp_rpi_inst="-b"
smp_rpi_inst: ${IMAGE.instk}/netbsd-RPI_INSTALL.bin.gz __mkimage
.if empty(ALL_KERNELS) || !empty(ALL_KERNELS:MRPI_INSTALL)
SNAP_MD_POST_DEPS+=	smp_rpi_inst
.endif
.endif

snap_md_post: check_DESTDIR check_RELEASEDIR snap_post .WAIT ${SNAP_MD_POST_DEPS}

KERNEL_SUFFIXES=	bin srec ub

INSTALLATION_DIRS+=	binary/gzimg		\
			installation/instkernel
