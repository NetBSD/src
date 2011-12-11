#	$NetBSD: Makefile.inc,v 1.9 2011/12/11 14:59:24 joerg Exp $

.include <bsd.own.mk>

BINDIR=	/usr/mdec

OPT_SIZE.gcc=	-Os -ffreestanding -fomit-frame-pointer
OPT_SIZE.clang=	-Oz -ffreestanding -fomit-frame-pointer -DNDEBUG \
		-fno-stack-protector -mno-sse \
		-mllvm -enable-iv-rewrite=true \
		-mstack-alignment=4 \
		-mllvm -inline-threshold=3 -mllvm -enable-load-pre=false
