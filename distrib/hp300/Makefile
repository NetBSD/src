#	$NetBSD: Makefile,v 1.13 2002/05/02 18:02:34 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

SUBDIR=		ramdisk .WAIT instkernel
TARGETS+=	release

release: check_RELEASEDIR .WAIT HP-IB.geometry
	${RELEASE_INSTALL} HP-IB.geometry ${RELEASEDIR}/installation/misc

.include <bsd.subdir.mk>
