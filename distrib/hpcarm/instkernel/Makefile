#	$NetBSD: Makefile,v 1.3 2010/09/04 01:16:49 tsutsui Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

MINIROOTOBJ!=	cd ${DISTRIBDIR}/miniroot && ${PRINTOBJDIR}
MINIROOT=	${MINIROOTOBJ}/miniroot.fs

MDSETTARGETS=		INSTALL_IPAQ	${MINIROOT}	netbsd-IPAQ \
			INSTALL_JORNADA720 ${MINIROOT}	netbsd-JORNADA720 \
			INSTALL_WZERO3	${MINIROOT}	netbsd-WZERO3
MDSET_RELEASEDIR=	installation

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
