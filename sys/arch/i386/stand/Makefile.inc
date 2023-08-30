#	$NetBSD: Makefile.inc,v 1.19 2023/08/30 18:45:46 christos Exp $

NOLIBCSANITIZER=
NOSANITIZER=
NOPIE=
NOMAN=

.include <bsd.own.mk>

BINDIR=	/usr/mdec

OPT_SIZE.gcc=	-Os -ffreestanding -fomit-frame-pointer -fno-unwind-tables \
		-fno-asynchronous-unwind-tables -fno-exceptions -mno-sse \
		-fno-stack-protector
OPT_SIZE.clang=	${OPT_SIZE.gcc} -mstack-alignment=4  -DNDEBUG
