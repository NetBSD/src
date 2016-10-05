#	$NetBSD: Makefile,v 1.1.2.2 2016/10/05 20:56:07 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	mlx
IOCONF=	mlx.ioconf

SRCS+=	mlx.c

CPPFLAGS+=	-DNLD=1

.include <bsd.kmodule.mk>
