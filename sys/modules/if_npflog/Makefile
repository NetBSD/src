# $NetBSD: Makefile,v 1.1.40.1 2018/09/30 01:45:55 pgoyette Exp $

.include "../Makefile.inc"

.PATH:		${S}/net/npf

KMOD=		if_npflog

SRCS=		if_npflog.c

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

.include <bsd.kmodule.mk>
