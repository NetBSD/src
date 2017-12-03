#	$NetBSD: Makefile.inc,v 1.3.178.1 2017/12/03 11:36:25 jdolecek Exp $

BINDIR=		/usr/mdec

CFLAGS_UNWIND.gcc=	-fno-unwind-tables -fno-asynchronous-unwind-tables
CFLAGS_UNWIND.clang=	-fno-unwind-tables

LINKFLAGS_UNWIND=	--no-ld-generated-unwind-info
