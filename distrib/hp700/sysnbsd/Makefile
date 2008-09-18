#	$NetBSD: Makefile,v 1.1.14.1 2008/09/18 04:41:09 wrstuden Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

MKBOOT = ${TOOLDIR}/bin/${_TOOL_PREFIX}hp700-mkboot

.include <bsd.kernobj.mk>

KERN=	${KERNOBJDIR}/GENERIC/netbsd
LIF=	SYSNBSD

release: check_RELEASEDIR 
	${MKBOOT} ${DESTDIR}/usr/mdec/boot ${KERN} ${.OBJDIR}/${LIF}
	${RELEASE_INSTALL} ${.OBJDIR}/${LIF} \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation

.include <bsd.prog.mk>
