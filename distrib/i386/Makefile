#	$NetBSD: Makefile,v 1.13.22.1 2026/11/04 22:36:54 perseant Exp $

.include <bsd.own.mk>

SUBDIR=
SUBDIR+=	ramdisks
SUBDIR+=	.WAIT
SUBDIR+=	instkernel
.if ${MKKMOD} != "no"
SUBDIR+=	kmod
SUBDIR+=	kmod-cgdroot
.endif
SUBDIR+=	.WAIT
SUBDIR+=	cdroms
SUBDIR+=	floppies
SUBDIR+=	liveimage
SUBDIR+=	installimage
TARGETS+=	release

iso_image: .MAKE
	${MAKEDIRTARGET} cdroms iso_image

live_image: .MAKE
	${MAKEDIRTARGET} liveimage live_image

install_image: .MAKE
	${MAKEDIRTARGET} installimage install_image

.include <bsd.subdir.mk>
