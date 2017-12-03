#	$NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:38:58 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	mlx
IOCONF=	mlx.ioconf

SRCS+=	mlx.c

CPPFLAGS+=	-DNLD=1

.include <bsd.kmodule.mk>
