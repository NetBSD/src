#	$NetBSD: Makefile,v 1.1.4.2 2016/11/04 14:49:19 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/eisa

KMOD=	mlx_eisa
IOCONF=	mlx_eisa.ioconf

SRCS+=	mlx_eisa.c

.include <bsd.kmodule.mk>
