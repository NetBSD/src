#	$NetBSD: Makefile,v 1.7 2008/11/21 15:49:51 ad Exp $

SUBDIR=
SUBDIR+=	ramdisks
SUBDIR+=	.WAIT
SUBDIR+=	instkernel
SUBDIR+=	.WAIT
SUBDIR+=	cdroms
SUBDIR+=	floppies
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
