#	$NetBSD: Makefile.inc,v 1.1 2002/03/17 22:14:18 tshiozak Exp $

# sources
.PATH: ${ARCHDIR}/citrus ${.CURDIR}/citrus

CITRUS?=	yes

.if ${CITRUS} == "yes"
MODULE_SHLIB_MAJOR!= . ${.CURDIR}/../i18n_module/shlib_version ; echo $$major
CPPFLAGS+= -DI18NMODULE_MAJOR=${MODULE_SHLIB_MAJOR}

SRCS+=	citrus_ctype.c citrus_module.c citrus_none.c

# to be dynamically loaded - see lib/i18n_module
#SRCS+=	citrus_big5.c citrus_euc.c citrus_euctw.c citrus_iso2022.c \
#	citrus_mskanji.c citrus_utf8.c

.endif
