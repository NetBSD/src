# $NetBSD: Makefile.inc,v 1.4 2024/11/02 22:02:39 christos Exp $

.include <bsd.own.mk>

.include "${NETBSDSRCDIR}/external/bsd/zstd/Makefile.zstd"

USE_FORT?=	yes # complex string handling

LIBARCHIVEDIR=	${NETBSDSRCDIR}/external/bsd/libarchive/dist

CPPFLAGS+=	-I${NETBSDSRCDIR}/external/bsd/libarchive/include
CPPFLAGS+=	-DPLATFORM_CONFIG_H=\"config_netbsd.h\" ${ZSTDCPPFLAGS}

LIBARCHIVE_FE_DIR!=	cd ${.PARSEDIR}/lib/libarchive_fe && ${PRINTOBJDIR}
LIBARCHIVE_FE=		${LIBARCHIVE_FE_DIR}/libarchive_fe.a

# GCC applies the check even for same functions like strftime,
# completely defeating the purpose.
CWARNFLAGS.gcc+=	-Wno-format-nonliteral

WARNS?=	4
