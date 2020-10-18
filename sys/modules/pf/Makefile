# $NetBSD: Makefile,v 1.14 2020/10/18 15:02:12 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dist/pf/net ${S}/dist/pf/netinet

KMOD=   pf
IOCONF=	pf.ioconf
SRCS=	if_compat.c
SRCS+=	if_pflog.c
SRCS+=	pf.c
SRCS+=	pf_if.c
SRCS+=	pf_ioctl.c
SRCS+=	pf_mtag.c
SRCS+=	pf_norm.c
SRCS+=	pf_osfp.c
SRCS+=	pf_ruleset.c
SRCS+=	pf_table.c
SRCS+=	tcp_rndiss.c

CPPFLAGS+=	-I${S}/dist/pf -I${S} -DINET6 -DINET

WARNS=	3

.if defined(HAVE_GCC) && ${HAVE_GCC} >= 7 && ${ACTIVE_CC} == "gcc"
COPTS.pf.c+=	-Wno-error=implicit-fallthrough
.endif
CWARNFLAGS.pf.c+=   ${GCC_NO_ADDR_OF_PACKED_MEMBER}
CWARNFLAGS.pf_norm.c+=   ${GCC_NO_ADDR_OF_PACKED_MEMBER}
CWARNFLAGS.pf_table.c+=   ${GCC_NO_ADDR_OF_PACKED_MEMBER}

.include <bsd.kmodule.mk>
