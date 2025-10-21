#	$NetBSD: Makefile,v 1.2 2025/10/21 04:35:30 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	mlx
IOCONF=	mlx.ioconf

SRCS+=	mlx.c

CPPFLAGS+=	-DNLD=1

WARNS=	3	# -Wsign-compare issues

.include <bsd.kmodule.mk>
