#	$NetBSD: Makefile,v 1.7 2020/04/03 17:51:33 christos Exp $

PROG=		ar

COPTS.arparse.c = -Wno-stack-protector
CPPFLAGS+=-Dbin_dummy_emulation=bin_vanilla_emulation

.include "${.CURDIR}/../common/Makefile.prog"
