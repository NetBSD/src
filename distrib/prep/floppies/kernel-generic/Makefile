#	$NetBSD: Makefile,v 1.3 2002/04/26 15:50:44 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/Makefile.inc"

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
	-mkdir -p ${RELEASEDIR}/binary/kernel
	${RELINSTALL} ${KERNEL} ${RELEASEDIR}/binary/kernel

.include <bsd.prog.mk>
