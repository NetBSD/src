# $NetBSD: Makefile,v 1.1 2002/04/11 16:33:58 skrll Exp $

.include <bsd.own.mk>

ITARGET=	${RELEASEDIR}/installation
IINST=		${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m ${NONBINMODE}

SUBDIR=		ramdisk .WAIT instkernel

.include <bsd.subdir.mk>

INSTOBJ!=	cd ${.CURDIR}/instkernel && ${PRINTOBJDIR}

release: check_RELEASEDIR
	${IINST} ${INSTOBJ}/netbsd.${kern}.gz ${ITARGET}/
