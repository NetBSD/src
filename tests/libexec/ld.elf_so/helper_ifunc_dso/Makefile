# $NetBSD: Makefile,v 1.2 2023/07/05 22:42:47 riastradh Exp $

LIB=		h_helper_ifunc_dso
SRCS=		h_helper_ifunc.c

# MIPS BFD emits the shared object, but warns that dynamic IFUNC symbols
# are not supported.  The MIPS tests are skipped at runtime for that reason.
# GCC builds do not make linker warnings fatal; match that behaviour when
# building the same test helper with Clang.
MIPS_IFUNC_NO_FATAL_WARNINGS=	yes

.include "../helper.mk"
