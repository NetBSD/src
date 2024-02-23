#	$NetBSD: Makefile,v 1.1 2024/02/23 05:33:03 mrg Exp $

.include <bsd.init.mk>

# We don't install isl, just provide it for GCC.
LIBISPRIVATE?= yes

.if ${LIBISPRIVATE} != "yes"

INCS=		gitversion.h isl_config.h
INCSDIR=	/usr/include

.include <bsd.inc.mk>
.endif

SUBDIR=		isl

.include <bsd.subdir.mk>
