#	$NetBSD: Makefile.inc,v 1.8.6.1 2012/02/18 07:32:23 mrg Exp $

.include <bsd.own.mk>

BINDIR=	/usr/mdec

OPT_SIZE.gcc=	-Os -ffreestanding -fomit-frame-pointer
OPT_SIZE.clang=	-Oz -ffreestanding -fomit-frame-pointer -DNDEBUG \
		-fno-stack-protector -mno-sse \
		-mllvm -enable-iv-rewrite=true \
		-mstack-alignment=4 \
		-mllvm -inline-threshold=3 -mllvm -enable-load-pre=false
