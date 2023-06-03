# $NetBSD: Makefile,v 1.2 2023/06/03 21:33:02 lukem Exp $

USE_FORT=yes
LIBISPRIVATE=yes

.include <bsd.init.mk>

LIB=back_mdb

SLAPD= ${LDAP_DISTDIR}/servers/slapd
BACK_MDB=${SLAPD}/back-mdb
LMDB=${LDAP_DISTDIR}/libraries/liblmdb

.PATH: ${BACK_MDB} ${LMDB}

COPTS.mdb.c += ${CC_WNO_STRINGOP_OVERFLOW}

CPPFLAGS+=-I${SLAPD} -I${BACK_MDB} -I${LMDB}

SRCS += \
	add.c delete.c id2entry.c mdb.c nextid.c attr.c dn2entry.c \
	idl.c midl.c operational.c bind.c dn2id.c index.c modify.c \
	search.c compare.c extended.c init.c modrdn.c tools.c \
	config.c filterindex.c key.c monitor.c version.c \

.include <bsd.lib.mk>
