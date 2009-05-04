#	$NetBSD: Makefile,v 1.1.10.2 2009/05/04 08:14:09 yamt Exp $

.include "../Makefile.inc"

KMOD=	exec_elf32

CPPFLAGS+=	-DEXEC_ELF32 -DPAX_ASLR -DCOREDUMP

.PATH:	${S}/kern
SRCS=	exec_elf32.c core_elf32.c

.include <bsd.kmodule.mk>
