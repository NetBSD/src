# $NetBSD: Makefile,v 1.1.6.2 2013/02/25 00:30:00 tls Exp $

.include "../../Makefile.inc"

.PATH: ${NETBSDSRCDIR}/external/cddl/osnet/dev/profile

KMOD=		profile
SRCS=		profile.c

CPPFLAGS+=	-I${NETBSDSRCDIR}/external/cddl/osnet/sys \
		-I${NETBSDSRCDIR}/external/cddl/osnet/dev/profile \
		-I${NETBSDSRCDIR}/external/cddl/osnet/dist/uts/common

CPPFLAGS+=	-Wno-unknown-pragmas -Wno-cast-qual

.include <bsd.kmodule.mk>
