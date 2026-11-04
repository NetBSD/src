#       $NetBSD: Makefile.inc,v 1.3.58.1 2026/11/04 23:19:17 perseant Exp $

.PATH:	${.PARSEDIR}

DlfcnMakefileParseDir:=${.PARSEDIR}
CPPFLAGS+=	-I${NETBSDSRCDIR}/libexec/ld.elf_so
CPPFLAGS+=	-I${DlfcnMakefileParseDir}
SRCS+=		dlfcn_elf.c
