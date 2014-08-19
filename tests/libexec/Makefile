# $NetBSD: Makefile,v 1.3.8.1 2014/08/20 00:04:51 tls Exp $
#
.include "Makefile.inc"
.include <bsd.own.mk>

.if ${MACHINE_ARCH} != "m68000"
TESTS_SUBDIRS=		ld.elf_so
.endif

TESTSDIR=	${TESTSBASE}/libexec

.include <bsd.test.mk>
