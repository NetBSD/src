#	$NetBSD: Makefile.inc,v 1.7 2011/06/16 16:39:53 joerg Exp $

.include <bsd.own.mk>

BINDIR=	/usr/mdec

OPT_SIZE.gcc=	-Os -ffreestanding -fomit-frame-pointer
OPT_SIZE.clang=	-Oz -ffreestanding -fomit-frame-pointer \
		-fno-stack-protector -mno-sse \
		-mllvm -stack-alignment=4 -mllvm -realign-stack=false \
		-mllvm -inline-threshold=3 -mllvm -enable-load-pre=false
