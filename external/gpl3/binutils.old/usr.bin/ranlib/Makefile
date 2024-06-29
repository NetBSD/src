#	$NetBSD: Makefile,v 1.9 2024/06/29 16:36:17 christos Exp $

PROG=		ranlib

COPTS.arparse.c = -Wno-stack-protector
CPPFLAGS+=-Dbin_dummy_emulation=bin_vanilla_emulation

.include "${.CURDIR}/../common/Makefile.prog"
