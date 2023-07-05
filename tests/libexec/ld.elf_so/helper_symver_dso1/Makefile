# $NetBSD: Makefile,v 1.4 2023/07/05 22:42:48 riastradh Exp $

LIB=		h_helper_symver_dso
SRCS=		h_helper_symver_dso1.c

LIBDIR=		${TESTSBASE}/libexec/ld.elf_so/h_helper_symver_dso1
SHLIBDIR=	${TESTSBASE}/libexec/ld.elf_so/h_helper_symver_dso1
SHLIB_MAJOR=	1

FILESDIR=	${TESTSBASE}/libexec/ld.elf_so/h_helper_symver_dso1

LDFLAGS.lib${LIB} += -Wl,--version-script=${.CURDIR}/h_helper_symver_dso1.map

.include "../helper.mk"

