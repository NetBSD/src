#       $NetBSD: Makefile.inc,v 1.1.6.1 2002/08/27 23:49:29 nathanw Exp $

.PATH: ${.CURDIR}/dlfcn

.if ${OBJECT_FMT} == "ELF"
CPPFLAGS+=	-I${NETBSDSRCDIR}/libexec/ld.elf_so
CPPFLAGS+=	-I${.CURDIR}/dlfcn
SRCS+=		dlfcn_elf.c
.endif
