#	$NetBSD: Makefile,v 1.11.4.1 2017/04/21 16:51:07 bouyer Exp $

.include <bsd.own.mk>

SUBDIR=
SUBDIR+=	ramdisks
SUBDIR+=	.WAIT
SUBDIR+=	instkernel
.if ${MKKMOD} != "no"
SUBDIR+=	kmod
.if ${MKCRYPTO} != "no"
SUBDIR+=	kmod-cgdroot
.endif
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
