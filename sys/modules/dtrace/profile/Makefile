# $NetBSD: Makefile,v 1.5 2018/06/06 15:01:13 kamil Exp $

.include "../../Makefile.inc"

.PATH: ${NETBSDSRCDIR}/external/cddl/osnet/dev/profile

KMOD=		dtrace_profile
SRCS=		profile.c

CPPFLAGS+=	-I${NETBSDSRCDIR}/external/cddl/osnet/sys \
		-I${NETBSDSRCDIR}/external/cddl/osnet/dev/profile \
		-I${NETBSDSRCDIR}/external/cddl/osnet/dist/uts/common

CPPFLAGS+=	-Wno-unknown-pragmas -Wno-cast-qual

.if defined(HAVE_GCC)
CPPFLAGS+=	-Wno-discarded-qualifiers
.endif

.include <bsd.kmodule.mk>
