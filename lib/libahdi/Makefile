#	$NetBSD: Makefile,v 1.1 2000/02/05 20:40:35 jdc Exp $

.if ${MACHINE} == "atari"
LIB=	ahdi
SRCS=	build.c check.c cksum.c magic.c openraw.c read.c write.c writedtab.c
INCS=	libahdi.h
INCSDIR=/usr/include
.endif

MAN=	ahdi_buildlabel.3 ahdi_checklabel.3 ahdi_readlabel.3 \
	ahdi_writedisktab.3 ahdi_writelabel.3 ahdi.4
MANSUBDIR=      /atari

.if ${MACHINE} == "atari"
.include <bsd.lib.mk>
.else
.include <bsd.man.mk>
.endif
.include <bsd.subdir.mk>
