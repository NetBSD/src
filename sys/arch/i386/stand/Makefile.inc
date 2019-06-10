#	$NetBSD: Makefile.inc,v 1.15.2.1 2019/06/10 22:06:21 christos Exp $

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
