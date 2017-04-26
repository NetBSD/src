# $NetBSD: Makefile.inc,v 1.2.30.1 2017/04/26 02:52:25 pgoyette Exp $

.include <bsd.own.mk>

USE_FORT?=	yes # complex string handling

LIBARCHIVEDIR=	${NETBSDSRCDIR}/external/bsd/libarchive/dist

CPPFLAGS+=	-I${NETBSDSRCDIR}/external/bsd/libarchive/include
CPPFLAGS+=	-DPLATFORM_CONFIG_H=\"config_netbsd.h\"

LIBARCHIVE_FE_DIR!=	cd ${.PARSEDIR}/lib/libarchive_fe && ${PRINTOBJDIR}
LIBARCHIVE_FE=		${LIBARCHIVE_FE_DIR}/libarchive_fe.a

# GCC applies the check even for same functions like strftime,
# completely defeating the purpose.
CWARNFLAGS.gcc+=	-Wno-format-nonliteral

WARNS?=	4
