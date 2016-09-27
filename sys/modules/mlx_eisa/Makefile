#	$NetBSD: Makefile,v 1.1 2016/09/27 03:53:08 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/eisa

KMOD=	mlx_eisa
IOCONF=	mlx_eisa.ioconf

SRCS+=	mlx_eisa.c

.include <bsd.kmodule.mk>
