#	$NetBSD: Makefile.inc,v 1.6.8.1 2011/08/27 15:37:27 jym Exp $

.include <bsd.own.mk>

BINDIR=	/usr/mdec

OPT_SIZE.gcc=	-Os -ffreestanding -fomit-frame-pointer
OPT_SIZE.clang=	-Oz -ffreestanding -fomit-frame-pointer \
		-fno-stack-protector -mno-sse \
		-mllvm -stack-alignment=4 -mllvm -realign-stack=false \
		-mllvm -inline-threshold=3 -mllvm -enable-load-pre=false
