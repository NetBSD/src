#	$NetBSD: Makefile,v 1.11 2019/09/22 18:45:19 brad Exp $

.include <bsd.init.mk>

USE_SHLIBDIR=	yes

.PATH:  ${.CURDIR}/../../dist/lib/libnvpair
.PATH:  ${.CURDIR}/../../dist/common/nvpair

CPPFLAGS+=	-I${OSNETDIR}/sys \
		-I${OPENSOLARIS_USR_DISTDIR}/head \
		-I${OPENSOLARIS_SYS_DISTDIR}/common

CPPFLAGS+=	-Wall \
		-Wno-unknown-pragmas \
		-Wno-shadow \
		-Wno-format-security \
		-Wno-format-nonliteral \
		-Wno-cast-qual

LIB=	nvpair

SRCS=	libnvpair.c nvpair_alloc_system.c nvpair_alloc_fixed.c nvpair.c fnvpair.c nvpair_json.c

.include "../../Makefile.zfs"
.include <bsd.lib.mk>
