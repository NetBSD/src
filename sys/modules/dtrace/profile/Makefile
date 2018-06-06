# $NetBSD: Makefile,v 1.4 2018/06/06 14:57:38 kamil Exp $

.include "../../Makefile.inc"

.PATH: ${NETBSDSRCDIR}/external/cddl/osnet/dev/profile

KMOD=		dtrace_profile
SRCS=		profile.c

CPPFLAGS+=	-I${NETBSDSRCDIR}/external/cddl/osnet/sys \
		-I${NETBSDSRCDIR}/external/cddl/osnet/dev/profile \
		-I${NETBSDSRCDIR}/external/cddl/osnet/dist/uts/common

CPPFLAGS+=	-Wno-unknown-pragmas -Wno-cast-qual

.if defined(HAVE_LLVM)
CPPFLAGS+=	-Wno-discarded-qualifiers
.endif

.include <bsd.kmodule.mk>
