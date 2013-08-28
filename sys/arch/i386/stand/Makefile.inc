#	$NetBSD: Makefile.inc,v 1.10.4.1 2013/08/28 23:59:17 rmind Exp $

.include <bsd.own.mk>

BINDIR=	/usr/mdec

OPT_SIZE.gcc=	-Os -ffreestanding -fomit-frame-pointer -fno-unwind-tables
OPT_SIZE.clang=	-Oz -ffreestanding -fomit-frame-pointer -DNDEBUG \
		-fno-stack-protector -mno-sse \
		-mstack-alignment=4 \
		-mllvm -inline-threshold=3 -mllvm -enable-load-pre=false \
		-fno-unwind-tables
