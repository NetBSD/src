#	$NetBSD: Makefile.inc,v 1.18.30.1 2023/12/14 17:56:08 martin Exp $

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
