#	$NetBSD: Makefile,v 1.1 2023/06/01 20:50:19 riastradh Exp $

.include <bsd.own.mk>

LIB=		h_use_dynamic
SRCS=		h_use_dynamic.c

DEF_DIR!=	cd ${.CURDIR}/../helper_def_dynamic && ${PRINTOBJDIR}
LDADD+=		-Wl,-rpath,${TESTSDIR} -L${DEF_DIR} -lh_def_dynamic

LIBDIR=		${TESTSBASE}/libexec/ld.elf_so
SHLIBDIR=	${TESTSBASE}/libexec/ld.elf_so
SHLIB_MAJOR=	1

MKSTATICLIB=	no
MKPROFILE=	no
MKPICINSTALL=	no
MKLINT=		no

NOMAN=		# defined

.include <bsd.lib.mk>
