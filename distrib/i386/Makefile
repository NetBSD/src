#	$NetBSD: Makefile,v 1.6 2008/05/02 23:13:05 ad Exp $

SUBDIR=
SUBDIR+=	ramdisks
SUBDIR+=	.WAIT
SUBDIR+=	instkernel
SUBDIR+=	kmod
SUBDIR+=	.WAIT
SUBDIR+=	cdroms
SUBDIR+=	floppies
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
