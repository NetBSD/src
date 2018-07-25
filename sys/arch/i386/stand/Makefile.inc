#	$NetBSD: Makefile.inc,v 1.16 2018/07/25 23:45:32 kamil Exp $

NOLIBCSANITIZER=
NOSANITIZER=
NOPIE=
NOMAN=

.include <bsd.own.mk>

BINDIR=	/usr/mdec

OPT_SIZE.gcc=	-Os -ffreestanding -fomit-frame-pointer -fno-unwind-tables \
		-fno-asynchronous-unwind-tables -fno-exceptions -mno-sse
OPT_SIZE.clang=	-Oz -ffreestanding -fomit-frame-pointer -DNDEBUG \
		-fno-stack-protector -mno-sse \
		-mstack-alignment=4 \
		-fno-unwind-tables
