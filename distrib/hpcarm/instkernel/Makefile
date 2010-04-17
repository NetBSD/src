#	$NetBSD: Makefile,v 1.2 2010/04/17 13:36:22 nonaka Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

MINIROOTOBJ!=	cd ${DISTRIBDIR}/miniroot && ${PRINTOBJDIR}
MINIROOT=	${MINIROOTOBJ}/miniroot.fs

MDSETTARGETS=		INSTALL_IPAQ	${MINIROOT}	netbsd-IPAQ \
			INSTALL_WZERO3	${MINIROOT}	netbsd-WZERO3
MDSET_RELEASEDIR=	installation

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
