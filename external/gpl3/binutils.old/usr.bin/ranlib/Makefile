#	$NetBSD: Makefile,v 1.6 2018/04/14 17:53:21 christos Exp $

PROG=		ranlib

COPTS.arparse.c = -Wno-stack-protector
CPPFLAGS+=-Dbin_dummy_emulation=bin_vanilla_emulation

.include "${.CURDIR}/../common/Makefile.prog"
