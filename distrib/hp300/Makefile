#	$NetBSD: Makefile,v 1.10 2002/04/14 20:48:16 gmcgarry Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

SUBDIR=		ramdisk .WAIT instkernel

TARGETS+=	release

release: check_RELEASEDIR
	-mkdir -p ${RELEASEDIR}/installation/misc
	${RELINSTALL} HP-IB.geometry ${RELEASEDIR}/installation/misc

.include <bsd.subdir.mk>
