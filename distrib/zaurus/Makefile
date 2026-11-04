# $NetBSD: Makefile,v 1.3.8.1 2026/11/04 22:37:12 perseant Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

SUBDIR=		ramdisk .WAIT instkernel cdroms
SUBDIR+=	liveimage
TARGETS+=	release

iso_image: .MAKE
	${MAKEDIRTARGET} cdroms iso_image

live_image: .MAKE
	${MAKEDIRTARGET} liveimage live_image

.include <bsd.subdir.mk>
