#	$NetBSD: Makefile,v 1.3.12.1 2006/04/22 02:58:41 simonb Exp $

MKPRIVATELIB=	yes

LIB=	parse
SRCS=	parse.c parse_conf.c clk_meinberg.c clk_schmid.c clk_rawdcf.c \
	clk_trimtsip.c clk_dcf7000.c clk_trimtaip.c clk_rcc8000.c \
	clk_hopf6021.c clk_computime.c clk_wharton.c clk_varitext.c \
	data_mbg.c info_trimble.c trim_info.c
NTPSRCDIR=lib${LIB}

.include <bsd.lib.mk>
