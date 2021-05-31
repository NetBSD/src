#	$NetBSD: Makefile,v 1.16.68.1 2021/05/31 22:06:53 cjep Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

.if ${MACHINE_ARCH} != "powerpc64"
SUBDIR=		floppies cdroms
.endif
TARGETS+=	release

BOOTOBJ!=	cd ${KERNSRCDIR}/arch/macppc/stand/ofwboot && ${PRINTOBJDIR}
OFWBOOTERS=	${BOOTOBJ}/ofwboot.elf ${BOOTOBJ}/ofwboot.xcf

release: check_RELEASEDIR .WAIT ${OWFBOOTERS}
	mkdir -p ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation
	${RELEASE_INSTALL} ${OFWBOOTERS} ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
