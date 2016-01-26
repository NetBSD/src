#	$NetBSD: Makefile,v 1.1.1.1 2016/01/26 17:27:00 christos Exp $

PROG=		ar

COPTS.arparse.c = -Wno-stack-protector
CPPFLAGS+=-Dbin_dummy_emulation=bin_vanilla_emulation

.include <bsd.own.mk>

.include "${.CURDIR}/../common/Makefile.prog"
