#	$NetBSD: Makefile,v 1.1 2024/02/23 05:33:04 mrg Exp $

.include <bsd.init.mk>

# We don't install isl, just provide it for GCC.
LIBISPRIVATE?= yes

.if ${LIBISPRIVATE} != "yes"

# Install libisl.  Only really used for mknative-gcc.

INCS=		stdint.h
INCSDIR=	/usr/include/isl

.BEGIN:
	-@mkdir -p ${DESTDIR}${INCSDIR}

.endif

NOPROG=
.include <bsd.prog.mk>
