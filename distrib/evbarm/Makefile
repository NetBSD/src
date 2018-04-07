#	$NetBSD: Makefile,v 1.9.40.1 2018/04/07 04:11:56 pgoyette Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

SUBDIR=		gzboot instkernel cdroms
TARGETS+=	release

.if ${MACHINE_CPU} == "arm"
BOOTOBJ!=	cd ${KERNSRCDIR}/arch/evbarm/stand/boot2440 && ${PRINTOBJDIR}
MDECBOOT=	${BOOTOBJ}/bootmini2440 

release: check_RELEASEDIR .WAIT ${MDECBOOT}
	${RELEASE_INSTALL} ${MDECBOOT} ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation
.endif

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
