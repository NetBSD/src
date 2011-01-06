#	$NetBSD: Makefile,v 1.1.4.2 2011/01/06 05:20:26 riz Exp $

.include <bsd.own.mk>
.include <bsd.init.mk>

CPPFLAGS+=	-DELFSIZE=32 -DELF32_COMPAT
LIB=	ldd_elf32_compat

# XXX Force one member
SRCS=	dummy.c

LIBISPRIVATE=	yes
.PATH: ${.CURDIR}/..

.ifdef COMPAT_MLIBDIR
MLIBDIR=	${COMPAT_MLIBDIR}
CPPFLAGS+=	-DRTLD_ARCH_SUBDIR=\"${MLIBDIR}\"
.endif

.ifdef MLIBDIR
.include "../Makefile.elf"
.endif

.include <bsd.lib.mk>
