#	$NetBSD: Makefile,v 1.16 2003/01/03 15:34:33 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

SUBDIR=		ramdisk .WAIT instkernel
TARGETS+=	release

release: check_RELEASEDIR .WAIT HP-IB.geometry
	${RELEASE_INSTALL} ${.CURDIR}/HP-IB.geometry \
	    ${RELEASEDIR}/${MACHINE}/installation/misc

.include <bsd.subdir.mk>
