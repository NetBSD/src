#	$NetBSD: Makefile,v 1.4 2002/05/02 18:02:49 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

KERN=		${KERNOBJDIR}/GENERIC/netbsd
KERNEL=		netbsd.GENERIC.gz

CLEANFILES+=	${KERNEL} netbsd.tmp netbsd.tmp.gz

${KERNEL}: ${KERN}
	cp ${KERN} netbsd.tmp
	gzip -9f netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}

realall: ${KERNEL}

release: check_RELEASEDIR .WAIT ${KERNEL}
	${RELEASE_INSTALL} ${KERNEL} ${RELEASEDIR}/binary/kernel

.include <bsd.prog.mk>
