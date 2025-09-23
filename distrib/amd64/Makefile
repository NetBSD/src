#	$NetBSD: Makefile,v 1.16 2025/09/23 20:12:05 rillig Exp $

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
SUBDIR+=	liveimage
SUBDIR+=	installimage
SUBDIR+=	installimage-bios
TARGETS+=	release 

iso_image: .MAKE
	${MAKEDIRTARGET} cdroms iso_image

live_image: .MAKE
	${MAKEDIRTARGET} liveimage live_image

install_image: .MAKE
	${MAKEDIRTARGET} installimage install_image
	${MAKEDIRTARGET} installimage-bios install_image

.include <bsd.subdir.mk>
