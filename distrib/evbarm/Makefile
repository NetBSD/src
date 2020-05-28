#	$NetBSD: Makefile,v 1.11 2020/05/28 15:23:43 jmcneill Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

SUBDIR=		gzboot instkernel cdroms
.if ${MACHINE_ARCH} == "aarch64" || ${MACHINE_ARCH} == "earmv7hf"
SUBDIR+=	installimage
.endif
TARGETS+=	release

.if ${MACHINE_CPU} == "arm"
BOOTOBJ!=	cd ${KERNSRCDIR}/arch/evbarm/stand/boot2440 && ${PRINTOBJDIR}
MDECBOOT=	${BOOTOBJ}/bootmini2440 

release: check_RELEASEDIR .WAIT ${MDECBOOT}
	${RELEASE_INSTALL} ${MDECBOOT} ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation
.endif

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

install_image:
	${MAKEDIRTARGET} installimage install_image

.include <bsd.subdir.mk>
