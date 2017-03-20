# $NetBSD: Makefile.inc,v 1.2.46.1 2017/03/20 06:52:19 pgoyette Exp $

.include <bsd.own.mk>

WARNS?=		4
BINDIR=		/sbin

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif

CWARNFLAGS.clang+=	-Wno-error=address-of-packed-member
