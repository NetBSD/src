#	$NetBSD: Makefile,v 1.8 2002/04/11 05:20:22 lukem Exp $

.include <bsd.own.mk>

SUBDIR=		ramdisk .WAIT instkernel

TARGETS+=	release

release: check_RELEASEDIR
	${INSTALL} ${PRESERVE} ${COPY} -m ${NONBINMODE} \
	    HP-IB.geometry ${RELEASEDIR}/installation/miniroot/

.include <bsd.subdir.mk>
