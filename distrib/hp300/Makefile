#	$NetBSD: Makefile,v 1.14 2002/05/05 15:05:33 jdolecek Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

SUBDIR=		ramdisk .WAIT instkernel
TARGETS+=	release

release: check_RELEASEDIR .WAIT HP-IB.geometry
	${RELEASE_INSTALL} ${.CURDIR}/HP-IB.geometry ${RELEASEDIR}/installation/misc

.include <bsd.subdir.mk>
