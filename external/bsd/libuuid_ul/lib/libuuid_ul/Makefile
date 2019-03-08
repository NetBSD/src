#	$NetBSD: Makefile,v 1.2 2019/03/08 09:58:23 mrg Exp $

# Makefile to build minimal util-linux libuuid locally, with special
# prefix on all functions to avoid conflicts with libc uuid_compare(3).
#

LIB=	uuid_ul

.include "srcs.mk"
SRCS=		${UUID_UL_SRCS}

.include "libuuid_ul-rename.mk"

LIBISPRIVATE=	yes

.include <bsd.lib.mk>

