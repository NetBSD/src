#	$NetBSD: Makefile.inc,v 1.12.8.1 2017/03/20 06:57:15 pgoyette Exp $

.include <bsd.own.mk>

BINDIR=	/usr/mdec

OPT_SIZE.gcc=	-Os -ffreestanding -fomit-frame-pointer -fno-unwind-tables \
		-fno-asynchronous-unwind-tables -fno-exceptions
OPT_SIZE.clang=	-Oz -ffreestanding -fomit-frame-pointer -DNDEBUG \
		-fno-stack-protector -mno-sse \
		-mstack-alignment=4 \
		-fno-unwind-tables
