#	$NetBSD: Makefile,v 1.17.4.1 2024/07/03 18:38:54 martin Exp $

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
# macppc installcd is now created via "do-isa-image" target in
# etc/Makefile using ${TOOLS_MKHYBRID} for ISO9660/HFS hybrid image
# with bootable special Apple partition map
#	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
