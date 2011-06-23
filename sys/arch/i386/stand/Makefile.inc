#	$NetBSD: Makefile.inc,v 1.6.26.1 2011/06/23 14:19:16 cherry Exp $

.include <bsd.own.mk>

BINDIR=	/usr/mdec

OPT_SIZE.gcc=	-Os -ffreestanding -fomit-frame-pointer
OPT_SIZE.clang=	-Oz -ffreestanding -fomit-frame-pointer \
		-fno-stack-protector -mno-sse \
		-mllvm -stack-alignment=4 -mllvm -realign-stack=false \
		-mllvm -inline-threshold=3 -mllvm -enable-load-pre=false
