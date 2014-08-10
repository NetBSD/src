# $NetBSD: Makefile,v 1.3.18.1 2014/08/10 06:57:24 tls Exp $
#
.include "Makefile.inc"
.include <bsd.own.mk>

.if ${MACHINE_ARCH} != "m68000"
TESTS_SUBDIRS=		ld.elf_so
.endif

TESTSDIR=	${TESTSBASE}/libexec

.include <bsd.test.mk>
