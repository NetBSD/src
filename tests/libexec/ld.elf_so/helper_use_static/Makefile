#	$NetBSD: Makefile,v 1.2 2023/05/31 00:46:11 riastradh Exp $

.include <bsd.own.mk>

LIB=		h_use_static
SRCS=		h_use_static.c

DEF_DIR!=	cd ${.CURDIR}/../helper_def_static && ${PRINTOBJDIR}
LDADD+=		-Wl,-rpath,${TESTSDIR} -L${DEF_DIR} -lh_def_static

LIBDIR=		${TESTSBASE}/libexec/ld.elf_so
SHLIBDIR=	${TESTSBASE}/libexec/ld.elf_so
SHLIB_MAJOR=	1

MKSTATICLIB=	no
MKPROFILE=	no
MKPICINSTALL=	no
MKLINT=		no

NOMAN=		# defined

.include <bsd.lib.mk>
