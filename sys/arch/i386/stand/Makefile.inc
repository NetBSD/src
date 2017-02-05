#	$NetBSD: Makefile.inc,v 1.12.4.1 2017/02/05 13:40:12 skrll Exp $

.include <bsd.own.mk>

BINDIR=	/usr/mdec

OPT_SIZE.gcc=	-Os -ffreestanding -fomit-frame-pointer -fno-unwind-tables \
		-fno-asynchronous-unwind-tables -fno-exceptions
OPT_SIZE.clang=	-Oz -ffreestanding -fomit-frame-pointer -DNDEBUG \
		-fno-stack-protector -mno-sse \
		-mstack-alignment=4 \
		-fno-unwind-tables
