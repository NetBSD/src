#	$NetBSD: Makefile,v 1.18.2.2 2026/11/04 22:36:55 perseant Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

.if ${MACHINE_ARCH} != "powerpc64"
SUBDIR=		floppies cdroms
SUBDIR+=	.WAIT
SUBDIR+=	liveimage
.endif
TARGETS+=	release

BOOTOBJ!=	cd ${KERNSRCDIR}/arch/macppc/stand/ofwboot && ${PRINTOBJDIR}
OFWBOOTERS=	${BOOTOBJ}/ofwboot.elf ${BOOTOBJ}/ofwboot.xcf

release: check_RELEASEDIR .WAIT ${OWFBOOTERS}
	mkdir -p ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation
	${RELEASE_INSTALL} ${OFWBOOTERS} ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation

iso_image: .MAKE
# macppc installcd is now created via "do-iso-image" target in
# etc/Makefile using ${TOOLS_MKHYBRID} for ISO9660/HFS hybrid image
# with bootable special Apple partition map
#	${MAKEDIRTARGET} cdroms iso_image

live_image: .MAKE
	${MAKEDIRTARGET} liveimage live_image

.include <bsd.subdir.mk>
