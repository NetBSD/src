# $NetBSD: Makefile,v 1.5 2017/04/22 05:32:40 pgoyette Exp $

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
