# $NetBSD: Makefile,v 1.2 2014/09/18 19:45:24 wiz Exp $

.include "../../Makefile.inc"

.PATH: ${NETBSDSRCDIR}/external/cddl/osnet/dev/profile

KMOD=		dtrace_profile
SRCS=		profile.c

CPPFLAGS+=	-I${NETBSDSRCDIR}/external/cddl/osnet/sys \
		-I${NETBSDSRCDIR}/external/cddl/osnet/dev/profile \
		-I${NETBSDSRCDIR}/external/cddl/osnet/dist/uts/common

CPPFLAGS+=	-Wno-unknown-pragmas -Wno-cast-qual

.include <bsd.kmodule.mk>
