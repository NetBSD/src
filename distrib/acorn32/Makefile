# $NetBSD: Makefile,v 1.17 2003/01/03 15:34:31 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

SUBDIR=		ramdisk .WAIT kernels stand
TARGETS+=	release

release: check_RELEASEDIR
	${RELEASE_INSTALL} ${DISTRIBDIR}/notes/acorn32/prep.RISCOS \
		    ${RELEASEDIR}/${MACHINE}/

.include <bsd.subdir.mk>
