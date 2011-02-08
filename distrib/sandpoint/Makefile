#	$NetBSD: Makefile,v 1.5.26.1 2011/02/08 16:18:35 bouyer Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

SUBDIR=		ramdisk .WAIT md-kernel cdroms
TARGETS+=	release

BOOTOBJ!=	cd ${KERNSRCDIR}/arch/sandpoint/stand/altboot && ${PRINTOBJDIR}
ALTBOOTERS=	${BOOTOBJ}/altboot ${BOOTOBJ}/altboot.bin

release: check_RELEASEDIR .WAIT ${ALTBOOTERS}
	${RELEASE_INSTALL} ${ALTBOOTERS} ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
