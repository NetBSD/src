#	$NetBSD: Makefile,v 1.9 2012/01/30 03:28:35 nisimura Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

SUBDIR=		gzboot instkernel cdroms
TARGETS+=	release

BOOTOBJ!=	cd ${KERNSRCDIR}/arch/evbarm/stand/boot2440 && ${PRINTOBJDIR}
MDECBOOT=	${BOOTOBJ}/bootmini2440 

release: check_RELEASEDIR .WAIT ${MDECBOOT}
	${RELEASE_INSTALL} ${MDECBOOT} ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
