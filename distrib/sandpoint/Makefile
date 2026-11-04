#	$NetBSD: Makefile,v 1.7.58.1 2026/11/04 22:36:58 perseant Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

SUBDIR=		ramdisk .WAIT md-kernel cdroms
TARGETS+=	release

BOOTOBJ!=	cd ${KERNSRCDIR}/arch/sandpoint/stand/altboot && ${PRINTOBJDIR}
ALTBOOTERS=	${BOOTOBJ}/altboot ${BOOTOBJ}/altboot.bin ${BOOTOBJ}/altboot.img

release: check_RELEASEDIR .WAIT ${ALTBOOTERS}
	${RELEASE_INSTALL} ${ALTBOOTERS} ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation

iso_image: .MAKE
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
