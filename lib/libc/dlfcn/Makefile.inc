#       $NetBSD: Makefile.inc,v 1.4 2026/10/04 03:28:42 mrg Exp $

.PATH:	${.PARSEDIR}

CPPFLAGS+=	-I${NETBSDSRCDIR}/libexec/ld.elf_so
CPPFLAGS+=	-I${.PARSEDIR}
SRCS+=		dlfcn_elf.c
