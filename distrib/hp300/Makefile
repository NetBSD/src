#	$NetBSD: Makefile,v 1.9 2002/04/11 08:49:48 lukem Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

SUBDIR=		ramdisk .WAIT instkernel

TARGETS+=	release

release: check_RELEASEDIR
	${RELINSTALL} HP-IB.geometry ${RELEASEDIR}/installation/miniroot/

.include <bsd.subdir.mk>
