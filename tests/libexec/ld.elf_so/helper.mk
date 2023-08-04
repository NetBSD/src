#	$NetBSD: helper.mk,v 1.1.4.2 2023/08/04 12:55:46 martin Exp $

# Makefile fragment for building a helper library used by ld.elf_so
# tests.  All we need to install is lib${LIB}.so and lib${LIB}.so.1.
# No man page, no lint library, no static libraries of any sort.

LIBDIR?=	${TESTSBASE}/libexec/ld.elf_so
SHLIBDIR?=	${TESTSBASE}/libexec/ld.elf_so
SHLIB_MAJOR?=	1

NODEBUGLIB=	# defined
NOLINT=		# defined
NOMAN=		# defined
NOPICINSTALL=	# defined
NOPROFILE=	# defined
NOSTATICLIB=	# defined

.include <bsd.lib.mk>
