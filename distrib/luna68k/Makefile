#	$NetBSD: Makefile,v 1.1 2011/07/16 15:52:20 tsutsui Exp $

SUBDIR=		ramdisk .WAIT instkernel .WAIT cdroms
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
