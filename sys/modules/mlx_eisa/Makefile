#	$NetBSD: Makefile,v 1.1.2.2 2016/10/05 20:56:07 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/eisa

KMOD=	mlx_eisa
IOCONF=	mlx_eisa.ioconf

SRCS+=	mlx_eisa.c

.include <bsd.kmodule.mk>
