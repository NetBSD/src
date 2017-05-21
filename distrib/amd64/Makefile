#	$NetBSD: Makefile,v 1.13 2017/05/21 15:28:37 riastradh Exp $

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
SUBDIR+=	uefi-installimage
TARGETS+=	release 

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

live_image:
	${MAKEDIRTARGET} liveimage live_image

install_image:
	${MAKEDIRTARGET} installimage install_image
	${MAKEDIRTARGET} uefi-installimage install_image

.include <bsd.subdir.mk>
