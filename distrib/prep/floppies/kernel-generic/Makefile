#	$NetBSD: Makefile,v 1.7.32.1 2008/06/29 08:25:13 mjf Exp $

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
	${RELEASE_INSTALL} ${KERNEL} ${RELEASEDIR}/${RELEASEMACHINEDIR}/binary/kernel

.include <bsd.prog.mk>
