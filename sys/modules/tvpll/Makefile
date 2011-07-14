#	$NetBSD: Makefile,v 1.1 2011/07/14 23:50:12 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/i2c

KMOD=	tvpll
SRCS=	tvpll.c tvpll_tuners.c

WARNS=	4

.include <bsd.kmodule.mk>
