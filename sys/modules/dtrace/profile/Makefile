# $NetBSD: Makefile,v 1.6 2018/06/07 13:21:44 kamil Exp $

.include "../../Makefile.inc"

.PATH: ${NETBSDSRCDIR}/external/cddl/osnet/dev/profile

KMOD=		dtrace_profile
SRCS=		profile.c

CPPFLAGS+=	-I${NETBSDSRCDIR}/external/cddl/osnet/sys \
		-I${NETBSDSRCDIR}/external/cddl/osnet/dev/profile \
		-I${NETBSDSRCDIR}/external/cddl/osnet/dist/uts/common

CPPFLAGS+=	-Wno-unknown-pragmas -Wno-cast-qual

CPPFLAGS+=	${${ACTIVE_CC} == "gcc" :? -Wno-discarded-qualifiers :}

.include <bsd.kmodule.mk>
