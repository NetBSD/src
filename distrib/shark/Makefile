#	$NetBSD: Makefile,v 1.1 2002/03/30 06:55:18 thorpej Exp $

.include <bsd.own.mk>		# Use /etc/mk.conf.

SUBDIR=	instkernel

ITARGET=	${RELEASEDIR}/installation
IINST=		${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m ${NONBINMODE}

INSTKERNOBJ!=	cd ${.CURDIR}/instkernel/instkernel && ${PRINTOBJDIR}

release: check_RELEASEDIR
	${IINST} ${INSTKERNOBJ}/netbsd.gz ${ITARGET}/instkernel/.

.include <bsd.subdir.mk>
