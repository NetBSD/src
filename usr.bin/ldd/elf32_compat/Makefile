#	$NetBSD: Makefile,v 1.1.48.1 2018/06/25 07:26:10 pgoyette Exp $

NOSANITIZER=		# defined

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
