#       $NetBSD: Makefile.inc,v 1.5 2026/10/05 04:43:56 kre Exp $

.PATH:	${.PARSEDIR}

DlfcnMakefileParseDir:=${.PARSEDIR}
CPPFLAGS+=	-I${NETBSDSRCDIR}/libexec/ld.elf_so
CPPFLAGS+=	-I${DlfcnMakefileParseDir}
SRCS+=		dlfcn_elf.c
