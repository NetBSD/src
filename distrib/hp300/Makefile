#	$NetBSD: Makefile,v 1.12 2002/04/26 15:50:30 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/Makefile.inc"

SUBDIR=		ramdisk .WAIT instkernel

TARGETS+=	release

release: check_RELEASEDIR
	${RELINSTALL} HP-IB.geometry ${RELEASEDIR}/installation/misc

.include <bsd.subdir.mk>
