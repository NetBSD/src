#	$NetBSD: Makefile,v 1.7 2002/03/17 05:41:58 gmcgarry Exp $

.include <bsd.own.mk>

SUBDIR=	ramdisk .WAIT instkernel

TARGETS+=release

release: check_RELEASEDIR
	${INSTALL} ${PRESERVE} ${COPY} -m ${NONBINMODE} \
	HP-IB.geometry ${RELEASEDIR}/installation/miniroot/

.include <bsd.subdir.mk>
