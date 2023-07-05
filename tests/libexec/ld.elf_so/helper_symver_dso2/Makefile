# $NetBSD: Makefile,v 1.4 2023/07/05 22:42:48 riastradh Exp $

LIB=		h_helper_symver_dso
SRCS=		h_helper_symver_dso2.c

LIBDIR=		${TESTSBASE}/libexec/ld.elf_so/h_helper_symver_dso2
SHLIBDIR=	${TESTSBASE}/libexec/ld.elf_so/h_helper_symver_dso2
SHLIB_MAJOR=	1

FILESDIR=	${TESTSBASE}/libexec/ld.elf_so/h_helper_symver_dso2

LDFLAGS.lib${LIB} += -Wl,--version-script=${.CURDIR}/h_helper_symver_dso2.map

.include "../helper.mk"
