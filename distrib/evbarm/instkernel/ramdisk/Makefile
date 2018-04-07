#	$NetBSD: Makefile,v 1.18.2.1 2018/04/07 04:11:56 pgoyette Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

IMAGE=		ramdisk.fs
IMAGESIZE=	3072k
MAKEFS_FLAGS+=	-f 15

WARNS=		1
DBG=		-Os
.if !empty(MACHINE_ARCH:Mearmv7*) && ${ACTIVE_CC} == "gcc"
DBG+=		-mthumb -mthumb-interwork
.endif

CRUNCHBIN=	ramdiskbin
LISTS=		${.CURDIR}/list ${DISTRIBDIR}/common/list.sysinst
MTREECONF=	${DISTRIBDIR}/common/mtree.common
.if empty(MACHINE_ARCH:M*eb)
IMAGEENDIAN=	le
.else
IMAGEENDIAN=	be
.endif
MAKEDEVTARGETS=	ramdisk
IMAGEDEPENDS=	${CRUNCHBIN} \
		dot.profile \
		${NETBSDSRCDIR}/etc/group ${NETBSDSRCDIR}/etc/master.passwd \
		${NETBSDSRCDIR}/etc/netconfig ${DISTRIBDIR}/common/protocols \
		${DISTRIBDIR}/common/services

.if !empty(MACHINE_ARCH:Maarch64)
UBOOT_IMAGE_ARCH=	arm64
.else
UBOOT_IMAGE_ARCH=	arm
.endif

# Use stubs to eliminate some large stuff from libc
HACKSRC=	${DISTRIBDIR}/utils/libhack
.include	"${HACKSRC}/Makefile.inc"
${CRUNCHBIN}:	libhack.o

.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.dhcpcd"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

RAMDISKDIR=${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/ramdisk

release:	${IMAGE}
	${INSTALL} -d -m 755 ${RAMDISKDIR}
	${RELEASE_INSTALL} ${IMAGE}  ${RAMDISKDIR}
	${TOOL_MKUBOOTIMAGE} -A ${UBOOT_IMAGE_ARCH} -C none -O linux \
	    -T ramdisk -a 0 -n "NetBSD/${MACHINE_ARCH} ramdisk" ${IMAGE} \
	    ${RAMDISKDIR}/${IMAGE:S/.fs/.ub/}

.include <bsd.prog.mk>
