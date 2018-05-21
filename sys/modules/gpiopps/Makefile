#	$NetBSD: Makefile,v 1.1.2.2 2018/05/21 04:36:15 pgoyette Exp $

.include "../Makefile.inc"

.PATH:		${S}/dev/gpio

KMOD=   	gpiopps
IOCONF=		gpiopps.ioconf
SRCS=		gpiopps.c

CPPFLAGS+=	-I${S}/gpio

.include <bsd.kmodule.mk>
