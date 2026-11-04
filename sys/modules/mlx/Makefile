#	$NetBSD: Makefile,v 1.1.58.1 2026/11/04 23:27:55 perseant Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	mlx
IOCONF=	mlx.ioconf

SRCS+=	mlx.c

CPPFLAGS+=	-DNLD=1

WARNS=	3	# -Wsign-compare issues

.include <bsd.kmodule.mk>
