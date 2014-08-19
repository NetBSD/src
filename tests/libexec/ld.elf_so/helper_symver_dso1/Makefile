# $NetBSD: Makefile,v 1.1.8.1 2014/08/20 00:04:51 tls Exp $

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

.include <bsd.lib.mk>

