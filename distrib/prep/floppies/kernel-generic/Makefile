#	$NetBSD: Makefile,v 1.2 2002/04/12 14:44:39 lukem Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

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
