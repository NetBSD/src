#	$NetBSD: Makefile,v 1.1 2008/11/19 18:36:08 ad Exp $

.include "../Makefile.inc"

KMOD=	exec_elf64

CPPFLAGS+=	-DEXEC_ELF64 -DPAX_ASLR -DCOREDUMP

.PATH:	${S}/kern
SRCS=	exec_elf64.c core_elf64.c

.include <bsd.kmodule.mk>
