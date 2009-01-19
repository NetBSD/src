#	$NetBSD: Makefile,v 1.1.6.2 2009/01/19 13:20:09 skrll Exp $

.include "../Makefile.inc"

KMOD=	exec_elf64

CPPFLAGS+=	-DEXEC_ELF64 -DPAX_ASLR -DCOREDUMP

.PATH:	${S}/kern
SRCS=	exec_elf64.c core_elf64.c

.include <bsd.kmodule.mk>
