#	$NetBSD: Makefile,v 1.1.2.2 2008/01/09 01:27:52 matt Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

MKBOOT = ${TOOLDIR}/bin/${_TOOL_PREFIX}hp700-mkboot

.include <bsd.kernobj.mk>

KERN=	${KERNOBJDIR}/GENERIC/netbsd
LIF=	SYSNBSD

release: check_RELEASEDIR 
	${MKBOOT} ${DESTDIR}/usr/mdec/boot ${KERN} ${.OBJDIR}/${LIF}
	${RELEASE_INSTALL} ${.OBJDIR}/${LIF} \
	    ${RELEASEDIR}/${MACHINE}/installation

.include <bsd.prog.mk>
