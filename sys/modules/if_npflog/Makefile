# $NetBSD: Makefile,v 1.1.42.1 2019/06/10 22:09:28 christos Exp $

.include "../Makefile.inc"

.PATH:		${S}/net/npf

KMOD=		if_npflog

SRCS=		if_npflog.c

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

.include <bsd.kmodule.mk>
