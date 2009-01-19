#	$NetBSD: Makefile,v 1.1.6.2 2009/01/19 13:20:08 skrll Exp $

.include "../Makefile.inc"

KMOD=	exec_elf32

CPPFLAGS+=	-DEXEC_ELF32 -DPAX_ASLR -DCOREDUMP

.PATH:	${S}/kern
SRCS=	exec_elf32.c core_elf32.c

.include <bsd.kmodule.mk>
