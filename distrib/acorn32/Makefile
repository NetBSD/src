# $NetBSD: Makefile,v 1.15.2.1 2002/07/15 17:22:14 thorpej Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

SUBDIR=		ramdisk .WAIT kernels stand
TARGETS+=	release

release: check_RELEASEDIR
	${RELEASE_INSTALL} ${DISTRIBDIR}/notes/acorn32/prep.RISCOS \
		    ${RELEASEDIR}/

.include <bsd.subdir.mk>
