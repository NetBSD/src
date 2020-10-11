#	$NetBSD: Makefile,v 1.13 2020/10/11 14:24:50 jmcneill Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

SUBDIR=		gzboot instkernel
.if ${MACHINE_ARCH} == "aarch64" || ${MACHINE_ARCH} == "earmv7hf"
SUBDIR+=	installimage
.endif
.if ${MACHINE_ARCH} == "aarch64"
SUBDIR+=	isoimage
.endif
TARGETS+=	release

.if ${MACHINE_CPU} == "arm"
BOOTOBJ!=	cd ${KERNSRCDIR}/arch/evbarm/stand/boot2440 && ${PRINTOBJDIR}
MDECBOOT=	${BOOTOBJ}/bootmini2440 

release: check_RELEASEDIR .WAIT ${MDECBOOT}
	${RELEASE_INSTALL} ${MDECBOOT} ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation
.endif

iso_image:
	${MAKEDIRTARGET} isoimage iso_image

install_image:
	${MAKEDIRTARGET} installimage install_image

.include <bsd.subdir.mk>
