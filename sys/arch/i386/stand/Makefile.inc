#	$NetBSD: Makefile.inc,v 1.8 2011/10/11 13:54:00 joerg Exp $

.include <bsd.own.mk>

BINDIR=	/usr/mdec

OPT_SIZE.gcc=	-Os -ffreestanding -fomit-frame-pointer
OPT_SIZE.clang=	-Oz -ffreestanding -fomit-frame-pointer -DNDEBUG \
		-fno-stack-protector -mno-sse \
		-mllvm -enable-iv-rewrite=true \
		-mllvm -stack-alignment=4 -mllvm -realign-stack=false \
		-mllvm -inline-threshold=3 -mllvm -enable-load-pre=false
