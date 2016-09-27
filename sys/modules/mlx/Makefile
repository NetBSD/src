#	$NetBSD: Makefile,v 1.1 2016/09/27 03:53:08 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	mlx
IOCONF=	mlx.ioconf

SRCS+=	mlx.c

CPPFLAGS+=	-DNLD=1

.include <bsd.kmodule.mk>
