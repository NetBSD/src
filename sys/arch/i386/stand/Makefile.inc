#	$NetBSD: Makefile.inc,v 1.8.2.2 2014/05/22 11:39:52 yamt Exp $

.include <bsd.own.mk>

BINDIR=	/usr/mdec

OPT_SIZE.gcc=	-Os -ffreestanding -fomit-frame-pointer -fno-unwind-tables \
		-fno-asynchronous-unwind-tables -fno-exceptions
OPT_SIZE.clang=	-Oz -ffreestanding -fomit-frame-pointer -DNDEBUG \
		-fno-stack-protector -mno-sse \
		-mstack-alignment=4 \
		-mllvm -inline-threshold=3 -mllvm -enable-load-pre=false \
		-fno-unwind-tables
