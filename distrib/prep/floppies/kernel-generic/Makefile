#	$NetBSD: Makefile,v 1.6 2002/11/18 12:39:54 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

KERN=		${KERNOBJDIR}/GENERIC/netbsd
KERNEL=		netbsd-GENERIC.gz

CLEANFILES+=	${KERNEL}

${KERNEL}: ${KERN}
	-rm -f ${.TARGET}
	gzip -9c ${.ALLSRC} > ${.TARGET}

realall: ${KERNEL}

release: check_RELEASEDIR .WAIT ${KERNEL}
	${RELEASE_INSTALL} ${KERNEL} ${RELEASEDIR}/binary/kernel

.include <bsd.prog.mk>
