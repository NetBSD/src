# $NetBSD: Makefile,v 1.2 2013/12/25 22:26:35 joerg Exp $

.include <bsd.own.mk>

LIB=		h_helper_symver_dso
SRCS=		h_helper_symver_dso1.c

LIBDIR=		${TESTSBASE}/libexec/ld.elf_so/h_helper_symver_dso1
SHLIBDIR=	${TESTSBASE}/libexec/ld.elf_so/h_helper_symver_dso1
SHLIB_MAJOR=	1

MKSTATICLIB=	no
MKPROFILE=	no
MKPICINSTALL=	no
MKLINT=		no

NOMAN=		# defined

FILESDIR=	${TESTSBASE}/libexec/ld.elf_so/h_helper_symver_dso1

LDFLAGS.lib${LIB} += -Wl,--version-script=${.CURDIR}/h_helper_symver_dso1.map

.if !empty(MACHINE_ARCH:M*arm*)
# LLVM PR 18273
COPTS.h_helper_symver_dso1.c=	${${ACTIVE_CC} == "clang":?-no-integrated-as :}
.endif

.include <bsd.lib.mk>

