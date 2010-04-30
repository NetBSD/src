# $NetBSD: Makefile,v 1.1.2.2 2010/04/30 14:44:18 uebayasi Exp $

.include "../Makefile.inc"

.PATH:	${S}/dist/pf/net ${S}/dist/pf/netinet

KMOD=   pf

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

.include <bsd.kmodule.mk>
