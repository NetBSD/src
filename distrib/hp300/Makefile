#	$NetBSD: Makefile,v 1.15 2002/05/07 03:29:43 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

SUBDIR=		ramdisk .WAIT instkernel
TARGETS+=	release

release: check_RELEASEDIR .WAIT HP-IB.geometry
	${RELEASE_INSTALL} ${.CURDIR}/HP-IB.geometry \
	    ${RELEASEDIR}/installation/misc

.include <bsd.subdir.mk>
