#	$NetBSD: Makefile,v 1.1 2023/06/02 19:08:49 riastradh Exp $

.include <bsd.own.mk>

LIB=		h_onlyuse_static
SRCS=		h_onlyuse_static.c

DEF_DIR!=	cd ${.CURDIR}/../helper_onlydef && ${PRINTOBJDIR}
LDADD+=		-Wl,-rpath,${TESTSDIR} -L${DEF_DIR} -lh_onlydef

LIBDIR=		${TESTSBASE}/libexec/ld.elf_so
SHLIBDIR=	${TESTSBASE}/libexec/ld.elf_so
SHLIB_MAJOR=	1

MKSTATICLIB=	no
MKPROFILE=	no
MKPICINSTALL=	no
MKLINT=		no

NOMAN=		# defined

.include <bsd.lib.mk>
