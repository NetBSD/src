#	$NetBSD: Makefile,v 1.7.6.1 2015/02/27 09:23:03 martin Exp $

SUBDIR=
SUBDIR+=	ramdisks
SUBDIR+=	.WAIT
SUBDIR+=	instkernel
SUBDIR+=	kmod
SUBDIR+=	kmod-cgdroot
SUBDIR+=	.WAIT
SUBDIR+=	cdroms
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
