#	$NetBSD: Makefile,v 1.2 2023/02/11 19:12:31 tsutsui Exp $

SUBDIR=		ramdisk .WAIT instkernel .WAIT cdroms
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
