#	$NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:38:58 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/eisa

KMOD=	mlx_eisa
IOCONF=	mlx_eisa.ioconf

SRCS+=	mlx_eisa.c

.include <bsd.kmodule.mk>
