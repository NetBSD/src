# $NetBSD: Makefile,v 1.2.24.1 2017/04/26 02:53:35 pgoyette Exp $

.include <bsd.own.mk>
PROG=	unzip

DPADD+=		${LIBARCHIVE} ${LIBZ} ${LIBBZ2}
LDADD+=		-larchive -lz -lbz2

.if ${MKCRYPTO:Uno} == "yes"
LDADD+=  -lcrypto
DPADD+=  ${LIBCRYPTO}
.endif

COPTS.unzip.c += -Wno-format-y2k

.include <bsd.prog.mk>
