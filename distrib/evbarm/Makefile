#	$NetBSD: Makefile,v 1.16 2021/07/23 21:53:33 jmcneill Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

SUBDIR=		gzboot instkernel
.if !empty(MACHINE_ARCH:Maarch64*) || !empty(MACHINE_ARCH:Mearmv7hf*)
SUBDIR+=	installimage
SUBDIR+=	liveimage
.endif
.if !empty(MACHINE_ARCH:Maarch64*)
SUBDIR+=	isoimage
.endif
TARGETS+=	release

.if ${MACHINE_CPU} == "arm"
BOOTOBJ!=	cd ${KERNSRCDIR}/arch/evbarm/stand/boot2440 && ${PRINTOBJDIR}
MDECBOOT=	${BOOTOBJ}/bootmini2440 

release: check_RELEASEDIR .WAIT ${MDECBOOT}
	${RELEASE_INSTALL} ${MDECBOOT} ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation
.endif

.if !empty(MACHINE_ARCH:Maarch64*)
iso_image:
	${MAKEDIRTARGET} isoimage iso_image
.endif

live_image:
	${MAKEDIRTARGET} liveimage live_image

install_image:
	${MAKEDIRTARGET} installimage install_image

.include <bsd.subdir.mk>
