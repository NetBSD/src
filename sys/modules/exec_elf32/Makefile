#	$NetBSD: Makefile,v 1.1.4.2 2009/01/17 13:29:29 mjf Exp $

.include "../Makefile.inc"

KMOD=	exec_elf32

CPPFLAGS+=	-DEXEC_ELF32 -DPAX_ASLR -DCOREDUMP

.PATH:	${S}/kern
SRCS=	exec_elf32.c core_elf32.c

.include <bsd.kmodule.mk>
