#	$NetBSD: Makefile,v 1.2 2024/02/08 19:24:43 christos Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

MKBOOT = ${TOOLDIR}/bin/${_TOOL_PREFIX}hppa-mkboot

.include <bsd.kernobj.mk>

KERN=	${KERNOBJDIR}/GENERIC/netbsd
LIF=	SYSNBSD

.if ${MKREPRO_TIMESTAMP:Uno} != "no"
MKBOOT_TIMESTAMP=-t "${MKREPRO_TIMESTAMP}"
.endif

release: check_RELEASEDIR 
	${MKBOOT} ${MKBOOT_TIMESTAMP} ${DESTDIR}/usr/mdec/boot ${KERN} ${.OBJDIR}/${LIF}
	${RELEASE_INSTALL} ${.OBJDIR}/${LIF} \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation

.include <bsd.prog.mk>
