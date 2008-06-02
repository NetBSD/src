#	$NetBSD: Makefile,v 1.1.6.2 2008/06/02 13:21:13 mjf Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

MINIROOTOBJ!=	cd ${DISTRIBDIR}/miniroot && ${PRINTOBJDIR}
MINIROOT=	${MINIROOTOBJ}/miniroot.fs

MDSETTARGETS=		RAMDISK		${MINIROOT}	netbsd		\
			INSTALL_TX3912	${MINIROOT}	netbsd-TX3912
MDSET_RELEASEDIR=	installation

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
