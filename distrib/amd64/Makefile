#	$NetBSD: Makefile,v 1.5 2011/01/18 00:22:56 jym Exp $

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
