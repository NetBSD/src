#       $NetBSD: Makefile.inc,v 1.1 2000/04/02 15:35:51 minoura Exp $

.PATH: ${.CURDIR}/dlfcn

.if ${OBJECT_FMT} == "ELF"
CPPFLAGS+=	-I${.CURDIR}/../../libexec/ld.elf_so
CPPFLAGS+=	-I${.CURDIR}/dlfcn
SRCS+=		dlfcn_elf.c
.endif
