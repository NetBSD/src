# $NetBSD: Makefile.inc,v 1.1.1.1 2024/08/25 06:38:36 wiz Exp $

.include <bsd.own.mk>

.include "${.PARSEDIR}/../Makefile.inc"

LIBINSTALL != cd ${.PARSEDIR}/../lib && ${PRINTOBJDIR}

BINDIR?=	/usr/sbin
CPPFLAGS+=	-DBINDIR='"${BINDIR}"'

DPADD+=	${LIBINSTALL}/libinstall.a
LDADD+=	-L${LIBINSTALL} -linstall -ltermcap

DPADD+=		${LIBNETPGPVERIFY} ${LIBFETCH} ${LIBSSL} ${LIBCRYPTO}
LDADD+=		-lnetpgpverify -lfetch -lssl -lcrypto

DPADD+=		${LIBARCHIVE}
LDADD+=		-larchive

DPADD+=		${LIBZ} ${LIBBZ2}
LDADD+=		-lz -lbz2

DPADD+=		${LIBLZMA} ${LIBPTHREAD}
LDADD+=		-llzma -lpthread
