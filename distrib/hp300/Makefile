#	$NetBSD: Makefile,v 1.11 2002/04/22 03:37:12 gmcgarry Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

SUBDIR=		ramdisk .WAIT instkernel

TARGETS+=	release

release: check_RELEASEDIR
	${RELINSTALL} HP-IB.geometry ${RELEASEDIR}/installation/misc

.include <bsd.subdir.mk>
