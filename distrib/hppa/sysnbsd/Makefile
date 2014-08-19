#	$NetBSD: Makefile,v 1.1.8.2 2014/08/19 23:45:40 tls Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

MKBOOT = ${TOOLDIR}/bin/${_TOOL_PREFIX}hppa-mkboot

.include <bsd.kernobj.mk>

KERN=	${KERNOBJDIR}/GENERIC/netbsd
LIF=	SYSNBSD

release: check_RELEASEDIR 
	${MKBOOT} ${DESTDIR}/usr/mdec/boot ${KERN} ${.OBJDIR}/${LIF}
	${RELEASE_INSTALL} ${.OBJDIR}/${LIF} \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation

.include <bsd.prog.mk>
