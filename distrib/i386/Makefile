#	$NetBSD: Makefile,v 1.11 2015/07/19 10:16:55 martin Exp $

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

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

live_image:
	${MAKEDIRTARGET} liveimage live_image

install_image:
	${MAKEDIRTARGET} installimage install_image

.include <bsd.subdir.mk>
