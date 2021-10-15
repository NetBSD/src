#	$NetBSD: Makefile,v 1.13.12.2 2021/10/15 15:14:51 martin Exp $

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

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

live_image:
	${MAKEDIRTARGET} liveimage live_image

install_image:
	${MAKEDIRTARGET} installimage install_image
	${MAKEDIRTARGET} installimage-bios install_image

.include <bsd.subdir.mk>
